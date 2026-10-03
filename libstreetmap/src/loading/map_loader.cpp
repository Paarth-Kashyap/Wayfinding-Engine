#include "global.h"
#include "spatial_grid.h"

/* map_loader.cpp
   Loading-domain code moved from m1.cpp (no behavior change).
   Owns all global variable DEFINITIONS plus loadMap/closeMap and load_* helpers. */

// Spatial acceleration grids over the segment/feature arrays (see spatial_grid.h).
SpatialGrid g_segment_grid;
SpatialGrid g_feature_grid;

// Builds both grids from the already-loaded segments/features. Called at the
// end of loadMap and whenever the data is (re)loaded.
void buildSpatialGrids(){
   // ~ one cell per few hundred items keeps buckets small but memory modest.
   int seg_cells  = std::max(64, (int)(segments.size() / 24));
   int feat_cells = std::max(16, (int)(features.size() / 8));

   g_segment_grid.init(min_x, min_y, max_x, max_y, seg_cells);
   for (size_t i = 0; i < segments.size(); ++i)
      g_segment_grid.insert((int)i, segments[i].seg_rect);

   g_feature_grid.init(min_x, min_y, max_x, max_y, feat_cells);
   for (size_t i = 0; i < features.size(); ++i)
      g_feature_grid.insert((int)i, features[i].feature_rect);
}

int loadCount = 0;
double min_lat;
double max_lat;
double avg_lat;
double avg_lat_cos;
double min_lon;
double max_lon;
double min_x;
double max_x;
double min_y;
double max_y;
double max_speed_limit; 
double oneWayAngleCoef;

/* Global Data Structure and variables */
std::vector<std::vector<StreetSegmentIdx>> intersection_street_segments; //holds intersections and thier segments 
std::vector<StreetSegmentIdx> street_segs;  // vector with all streetSegmants
std::vector<std::vector<StreetSegmentIdx>> streets;  // holds [street][street_seg of that street]
std::vector<double> streets_length;  // holds length of each street (the location of length represents streetIdx)
std::unordered_map<OSMID, const OSMWay*> osmWayMap; //stores OSMWay objects based on OSMID
std::unordered_map<OSMID, const OSMNode*> osmNodeMap; //stores OSMNode objects based on OSMID
std::vector<double> street_seg_travel_time; //vector with all street segments' travel times [StreetSegmentIdx][travel_time of seg]
std::vector<std::vector<IntersectionIdx>> street_intersections; //vector with all streets and their intersections [street][intersection in street]
std::unordered_map<OSMID, std::vector<std::pair<std::string, std::string>>> osmNode_pairs;
std::unordered_map<std::string, std::vector<StreetIdx>> prefixMap;

//data strutures used in m2.cpp
std::vector<intersection_data> intersections;
std::vector<street_data> streets_struc_vec;
std::vector<seg_data> segments;
std::unordered_map<OSMID, std::string> osmRoadMap;
std::vector<osm_data> osmSubwayStations;
std::vector<osm_data> osmTrafficLights;
std::vector<osm_data> osmStopSigns;
std::vector<feature_data> features;
std::vector<POI_data> pois; //vector with all POIs and their data
std::vector<POI_data> school_pois; //vector with all POIs and their data
std::vector<POI_data> food_pois; //vector with all POIs and their data
std::vector<POI_data> emergency_pois; //vector with all POIs and their data
std::vector<POI_data> leisure_pois; //vector with all POIs and their data
std::vector<POI_data> driving_pois; //vector with all POIs and their data

//png images
ezgl::surface * traffic_light_icon;
ezgl::surface * stop_sign_icon;
ezgl::surface * subway_icon;
ezgl::surface * school_icon;
ezgl::surface * food_icon;
ezgl::surface * emergency_icon;
ezgl::surface * leisure_icon;
ezgl::surface * driving_icon;
ezgl::surface* destination_icon;



//--------------------------------------------------------------------------
bool loadMap(std::string map_streets_database_filename) {
  bool load_successful=false;
  //to prevent loading everything twice 
  if (loadCount > 1) {
    return load_successful;
  }
  if(loadStreetsDatabaseBIN(map_streets_database_filename)){

    std::cout << "loadMap: " << map_streets_database_filename << std::endl;
    //extract city name from the file name
    std::string city_name = map_streets_database_filename.substr(0, map_streets_database_filename.find(".streets.bin"));
    if (loadOSMDatabaseBIN(city_name + ".osm.bin")){
      loadCount++;

      //get min max avg lat and lon
      max_lat=getIntersectionPosition(0).latitude();
      min_lat=max_lat;
      max_lon=getIntersectionPosition(0).longitude();
      min_lon=max_lon;
      loadLatLonStuff();
      avg_lat= (min_lat + max_lat) / 2;
      avg_lat_cos=cos(avg_lat*kDegreeToRadian);
      oneWayAngleCoef=cos(45);
      min_x = positionX(min_lon);
      max_x = positionX(max_lon);
      min_y = positionY(min_lat);
      max_y = positionY(max_lat);

      // Parallel load in dependency stages. Within a stage, every loader
      // writes to its own pre-sized / disjoint structures, so the StreetsDB
      // read API is only ever called for reads concurrently and no two
      // threads write the same memory.
      //
      // Stage A: independent structures.
      //   - load_OSMWayMap builds osmRoadMap (needed by Stage B's road class).
      {
        std::thread tA1(load_OSMWayMap);
        std::thread tA2(load_OSMNodeMap);
        std::thread tA3(load_PrefixMap);
        std::thread tA4(load_intersection_street_segs); // sizes `intersections`
        tA1.join(); tA2.join(); tA3.join(); tA4.join();
      }

      // Stage B: sizes/populates `streets`, `segments`, `streets_struc_vec`;
      // reads osmRoadMap for the per-segment road class, so it must follow A.
      load_Street_and_Segment();

      // Stage C: derived per-street / per-segment data. Each loader writes
      // distinct members of streets_struc_vec[i] (distinct addresses, no
      // reallocation) and its own global vector, so they are race-free.
      {
        std::thread tC1(load_streets_length);
        std::thread tC2(load_street_seg_travel_time);
        std::thread tC3(load_street_intersections);
        tC1.join(); tC2.join(); tC3.join();
      }

      // Stage D: POIs, features, and image surfaces are mutually independent.
      {
        std::thread tD1(load_POIs);
        std::thread tD2(load_features);
        load_images(); // keep GTK/cairo surface loading on the main thread
        tD1.join(); tD2.join();
      }

      // Build spatial acceleration grids now that segments/features exist.
      buildSpatialGrids();

      return true;
    } 
  }
  return false;
}

void closeMap() {
  // Clean-up your map related data structures here
  //close all vectors and data structures global variables

  std::vector<double>().swap(streets_length);
  std::vector<double>().swap(street_seg_travel_time);
  std::vector<StreetSegmentIdx>().swap(street_segs);



  std::vector<StreetSegmentIdx>().swap(my_path);
  my_path_start_end.first = -1;
  my_path_start_end.second = -1;

  //2d vectors clearing
  std::vector<std::vector<StreetSegmentIdx>>().swap(streets);
  std::vector<std::vector<StreetSegmentIdx>>().swap(intersection_street_segments);
  std::vector<std::vector<IntersectionIdx>>().swap(street_intersections);
  std::unordered_map<OSMID, const OSMWay*>().swap(osmWayMap);
  std::unordered_map<OSMID, const OSMNode*>().swap(osmNodeMap);
  std::unordered_map<OSMID, std::vector<std::pair<std::string, std::string>>>().swap(osmNode_pairs);
  std::unordered_map<std::string, std::vector<StreetIdx>>().swap(prefixMap);
  std::unordered_map<OSMID, std::string>().swap(osmRoadMap);

  //poi struct vector free
  std::vector<POI_data>().swap(pois);
  std::vector<POI_data>().swap(school_pois);
  std::vector<POI_data>().swap(food_pois);
  std::vector<POI_data>().swap(emergency_pois);
  std::vector<POI_data>().swap(leisure_pois);
  std::vector<POI_data>().swap(driving_pois);
  

  //feature struct vector free
  for(int i=0;i<features.size();i++){
    std::vector<LatLon>().swap(features[i].LatLon_locs);
    std::vector<ezgl::point2d>().swap(features[i].xy_locs);
  }
  std::vector<feature_data>().swap(features);

  //segment struct vector free
  for(int i=0;i<segments.size();i++){
    std::vector<LatLon>().swap(segments[i].curve_points);
    std::vector<ezgl::point2d>().swap(segments[i].xy_curve_points);
  }
  std::vector<seg_data>().swap(segments);

  //intersection struct vector free
  for(int i=0;i<intersections.size();i++){
    std::vector<StreetSegmentIdx>().swap(intersections[i].street_segs);
  }
  std::vector<intersection_data>().swap(intersections);

  //street struct vector free
  for(int i=0;i<streets_struc_vec.size();i++){
    std::vector<StreetSegmentIdx>().swap(streets_struc_vec[i].street_segs);
    std::vector<IntersectionIdx>().swap(streets_struc_vec[i].intersections);
  }
  std::vector<street_data>().swap(streets_struc_vec);

  //osm struct vector free
  std::vector<osm_data>().swap(osmSubwayStations);
  std::vector<osm_data>().swap(osmTrafficLights);
  std::vector<osm_data>().swap(osmStopSigns);
  
  close_images();

  closeStreetDatabase();
  closeOSMDatabase();

  // Release spatial grids so a reload rebuilds them for the new map.
  g_segment_grid.clear();
  g_feature_grid.clear();

  loadCount--;
}


//---------------------Helper Functions ------------------------

/* initialize street_segmant and street data structure */
void load_Street_and_Segment() {
	// Initialized vectors' length
	street_segs.resize(getNumStreetSegments());
	streets.resize(getNumStreets());

  streets_struc_vec.resize(getNumStreets()); //struct stuff
  segments.resize(getNumStreetSegments()); //struct stuff
	
	// Loop through vector (id of segments at each intersection)
	for (auto segmentIdx = 0; segmentIdx < street_segs.size(); ++segmentIdx) {
		// Load all street segments into vector
		street_segs[segmentIdx] = segmentIdx;
    StreetIdx streetID_temp=getStreetSegmentInfo(segmentIdx).streetID;
		// Places the current segment in its street (should be in sorted order)
		streets[streetID_temp].push_back(segmentIdx);

    //struct stuff
    segments[segmentIdx].start = getIntersectionPosition(getStreetSegmentInfo(segmentIdx).from);
    segments[segmentIdx].end = getIntersectionPosition(getStreetSegmentInfo(segmentIdx).to);
    segments[segmentIdx].length = findStreetSegmentLength(segmentIdx);
    segments[segmentIdx].speed = getStreetSegmentInfo(segmentIdx).speedLimit;
    segments[segmentIdx].id = segmentIdx;
    segments[segmentIdx].street_id = streetID_temp;
    segments[segmentIdx].one_way = getStreetSegmentInfo(segmentIdx).oneWay;
    segments[segmentIdx].name = getStreetName(streetID_temp);

    // Resolve and cache the road class once, so the per-frame draw loop never
    // calls getStreetSegmentInfo()/osmRoadMap lookups or compares strings.
    OSMID osm_id = getStreetSegmentInfo(segmentIdx).wayOSMID;
    segments[segmentIdx].road_class = classifyRoad(osmRoadMap[osm_id]);

    // Cache the endpoint xy so the draw path reuses them instead of projecting.
    segments[segmentIdx].start_xy = {positionX(segments[segmentIdx].start.longitude()),
                                     positionY(segments[segmentIdx].start.latitude())};
    segments[segmentIdx].end_xy   = {positionX(segments[segmentIdx].end.longitude()),
                                     positionY(segments[segmentIdx].end.latitude())};

    //get min and max xy locations of each segment (correct running bounds)
    ezgl::point2d min = segments[segmentIdx].start_xy;
    ezgl::point2d max = min;
    auto expand = [&min, &max](const ezgl::point2d& p){
      min.x = std::min(min.x, p.x);  max.x = std::max(max.x, p.x);
      min.y = std::min(min.y, p.y);  max.y = std::max(max.y, p.y);
    };
    //curve points struct stuff
    int numCurve = getStreetSegmentInfo(segmentIdx).numCurvePoints;
    segments[segmentIdx].curve_points.resize(numCurve);
    segments[segmentIdx].xy_curve_points.resize(numCurve);
    for (int i = 0; i < numCurve; i++) {
      segments[segmentIdx].curve_points[i] = getStreetSegmentCurvePoint(segmentIdx, i);
      ezgl::point2d cp = {positionX(segments[segmentIdx].curve_points[i].longitude()),
                          positionY(segments[segmentIdx].curve_points[i].latitude())};
      segments[segmentIdx].xy_curve_points[i] = cp;
      expand(cp);
    }
    expand(segments[segmentIdx].end_xy);

    segments[segmentIdx].seg_rect = ezgl::rectangle(min, max);
	}
}

// Maps an OSM road-type string to a RoadClass. Called once per segment at load.
RoadClass classifyRoad(const std::string& road_type){
  if (road_type == "motorway" || road_type == "trunk")
    return RoadClass::HIGHWAY;
  if (road_type == "trunk_link" || road_type == "motorway_link")
    return RoadClass::LINK;
  if (road_type == "primary" || road_type == "primary_link")
    return RoadClass::PRIMARY;
  if (road_type == "secondary" || road_type == "secondary_link")
    return RoadClass::SECONDARY;
  if (road_type == "tertiary" || road_type == "tertiary_link")
    return RoadClass::TERTIARY;
  if (road_type == "residential" || road_type == "pedestrian" ||
      road_type == "unclassified" || road_type == "service")
    return RoadClass::OTHER;
  return RoadClass::NONE;
}

/* loads the length of all streets into vector structure */
void load_streets_length() {
  streets_length.resize(getNumStreets()); // The streets_length vector holds length of each street
  // Loop through each street
  for (auto streetIdx = 0; streetIdx < getNumStreets(); streetIdx++) {

    //struct stuff
    streets_struc_vec[streetIdx].name = getStreetName(streetIdx);
    streets_struc_vec[streetIdx].id = streetIdx;
    streets_struc_vec[streetIdx].street_segs.resize(streets[streetIdx].size());
    streets_struc_vec[streetIdx].street_segs=streets[streetIdx];
    double segLength = 0;
    // Iterate through segments of street vector
    for (auto segIdx = 0; segIdx < streets[streetIdx].size(); segIdx++) {
      // Load total length into vector
      segLength += findStreetSegmentLength(streets[streetIdx][segIdx]);
    }
  streets_length.at(streetIdx)=segLength;  // Add to street_length vector
  streets_struc_vec[streetIdx].length = segLength; //struct stuff
  }
}


/* store all OSMWays in a hash map based on OSMID */
void load_OSMWayMap(){

  osmRoadMap.rehash(getNumberOfWays());
  for (unsigned int idx = 0; idx < getNumberOfWays(); idx++) {
    osmWayMap.insert({getWayByIndex(idx)->id(),getWayByIndex(idx)});

    const OSMWay* way = getWayByIndex(idx);
    int tag_count = getTagCount(way); 
    
    //check if way is a road of sometype
    for (int i=0; i<tag_count; i++){
      std::pair<std::string, std::string> tagPair = getTagPair(way, i);  // get actual tag pairs.

      if((tagPair.first == "highway" && tagPair.second == "motorway")
        || (tagPair.first == "highway" && tagPair.second == "trunk")
        || (tagPair.first == "highway" && tagPair.second == "primary")
        || (tagPair.first == "highway" && tagPair.second == "secondary")
        || (tagPair.first == "highway" && tagPair.second == "tertiary")
        || (tagPair.first == "highway" && tagPair.second == "unclassified")
        || (tagPair.first == "highway" && tagPair.second == "residential")
        || (tagPair.first == "highway" && tagPair.second == "motorway_link")
        || (tagPair.first == "highway" && tagPair.second == "trunk_link")
        || (tagPair.first == "highway" && tagPair.second == "primary_link")
        || (tagPair.first == "highway" && tagPair.second == "secondary_link")
        || (tagPair.first == "highway" && tagPair.second == "pedestrian")
        || (tagPair.first == "highway" && tagPair.second == "service")){
        osmRoadMap.emplace(getWayByIndex(idx)->id(), tagPair.second);
      } 
    }
    
  }
}


/* store all OSMNodes in a hash map based on OSMID */
void load_OSMNodeMap(){

  osmNode_pairs.rehash(getNumberOfNodes());
  for (unsigned int idx = 0; idx < getNumberOfNodes(); idx++) {
    
    //osm info
    LatLon coords=getNodeCoords(getNodeByIndex(idx));
    ezgl::point2d xy_coords={positionX(coords.longitude()),positionY(coords.latitude())};
    int num_tags=getTagCount(getNodeByIndex(idx));
    OSMID osm_id=getNodeByIndex(idx)->id();
    const OSMNode *node = getNodeByIndex(idx);


    osmNodeMap.insert({osm_id,node});

    std::vector<std::pair<std::string, std::string>> osmTagVector;
    osmTagVector.resize(num_tags);

    
    //loop through all tags of the node
    for (int i = 0; i < num_tags; i++){
      osmTagVector[i] = getTagPair(getNodeByIndex(idx), i);
      //store the traffic signals nodes
      if(getTagPair(getNodeByIndex(idx), i).first == "highway" && getTagPair(getNodeByIndex(idx), i).second == "traffic_signals"){
        
        osm_data temp={osm_id,num_tags,node,coords,xy_coords};
        osmTrafficLights.push_back(temp);
      }
      //store the stop sign nodes
      if(getTagPair(getNodeByIndex(idx), i).first == "highway" && getTagPair(getNodeByIndex(idx), i).second == "stop"){
        osm_data temp={osm_id,num_tags,node,coords,xy_coords};
        osmStopSigns.push_back(temp);
      }
      //store subway station nodes
      if (getTagPair(getNodeByIndex(idx), i).first == "station" && getTagPair(getNodeByIndex(idx), i).second == "subway") {
        osm_data temp={osm_id,num_tags,node,coords,xy_coords};
        osmSubwayStations.push_back(temp);
      }
    }

    //sort the tag vectors then add it to osmNode pairs
    std::sort(osmTagVector.begin(), osmTagVector.end());
    osmNode_pairs[getNodeByIndex(idx)->id()] = osmTagVector;

  }
}

/* loads the intersection_street_segments vector */
void load_intersection_street_segs(){
  intersection_street_segments.resize(getNumIntersections()); // Size the global vector to hold all intersections
  intersections.resize(getNumIntersections());  //struct stuff
  // Loop through all intersections
  for (auto intersecIdx=0; intersecIdx<getNumIntersections(); ++intersecIdx){
    //STRUCT STUFF
    //loading intersection structs
    intersections[intersecIdx].LatLon_loc = getIntersectionPosition(intersecIdx);
    intersections[intersecIdx].name = getIntersectionName(intersecIdx);
    intersections[intersecIdx].id = intersecIdx;  
    //get min and max lat and lon
    float x = positionX(intersections[intersecIdx].LatLon_loc.longitude());
    float y = positionY(intersections[intersecIdx].LatLon_loc.latitude());
    intersections[intersecIdx].xy_loc = ezgl::point2d(x,y); 


    // Loop through segments at each intersection
    //intersection_street_segments[intersecIdx].resize(getNumIntersectionStreetSegment(intersecIdx)); //struct stuff
    for (auto segmantIdx=0; segmantIdx<getNumIntersectionStreetSegment(intersecIdx); ++segmantIdx){
      // Add the number of segmants on each intersection to vector
      intersection_street_segments[intersecIdx].push_back(getIntersectionStreetSegment(intersecIdx, segmantIdx));
      //struct stuff
      intersections[intersecIdx].street_segs.push_back(getIntersectionStreetSegment(intersecIdx, segmantIdx));

    }
  }  
}


/* loads streets to be compared to prefix when called */
void load_PrefixMap(){
  // Compresses all street names
  for (int streetloop = 0; streetloop < getNumStreets(); streetloop++){
    std::string street = getStreetName(streetloop);
    std::string compressedName = compressedStreetName(street);

    // Compresses prefix name
    for (int alongstreet = 0; alongstreet < street.length(); alongstreet++) {
            std::string prefix = compressedName.substr(0, alongstreet + 1);
            prefixMap[prefix].emplace_back(streetloop);
    }
  }
    // Removes duplicates 
    for (auto begin = prefixMap.begin(); begin != prefixMap.end(); begin++){
      std::vector<int>& ids = begin->second;
      std::sort(ids.begin(), ids.end());
      ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    }
  
}

/* loads all street intersections to street_intersections vector */
void load_street_intersections(){
  // Adjusts sizes to not access wrong memory
  street_intersections.resize(streets.size());
  //Iterates for every street
  for(int i = 0; i < streets.size(); i++){
    // Iterates for every street segment
    for(int j = 0; j < streets[i].size(); j++){
      StreetSegmentInfo IntersectStreetSegInfo = getStreetSegmentInfo(streets[i][j]);
      street_intersections[i].push_back(IntersectStreetSegInfo.from);
      street_intersections[i].push_back(IntersectStreetSegInfo.to);
    }
    // Remove duplicates
    std::sort(street_intersections[i].begin(), street_intersections[i].end());
    street_intersections[i].erase(std::unique(street_intersections[i].begin(), street_intersections[i].end()), street_intersections[i].end());

    streets_struc_vec[i].intersections = street_intersections[i]; //struct stuff
  }
}

/* returns a double that determines the amount of travel time for one street segment*/
void load_street_seg_travel_time() {
  const int numSegs = getNumStreetSegments();
  // Pre-size so we index by SegIdx instead of push_back (push_back is not
  // thread-safe and reallocates, which would race with concurrent readers).
  street_seg_travel_time.resize(numSegs);

  // Accumulate the max speed locally, then publish once (no shared-write race).
  double local_max_speed = max_speed_limit;

  for(int SegIdx = 0; SegIdx < numSegs; SegIdx++) {
    float speed_limit = getStreetSegmentInfo(SegIdx).speedLimit;
    double distance = findStreetSegmentLength(SegIdx);

    if(speed_limit > local_max_speed){
      local_max_speed = speed_limit;
    }

    double travel_time = distance / static_cast<double>(speed_limit);
    street_seg_travel_time[SegIdx] = travel_time;
    segments[SegIdx].travel_time = travel_time; //struct stuff
  }

  max_speed_limit = local_max_speed;
}


//loads min max lat lon of the map
void loadLatLonStuff(){
  for(int idx = 0; idx < getNumIntersections(); idx++){
    max_lat = std:: max(max_lat, getIntersectionPosition(idx).latitude());
    min_lat = std:: min(min_lat, getIntersectionPosition(idx).latitude());
    max_lon = std:: max(max_lon, getIntersectionPosition(idx).longitude());
    min_lon = std:: min(min_lon, getIntersectionPosition(idx).longitude());
  }
}
//loads all POIS into vector
void load_POIs(){
  pois.resize(getNumPointsOfInterest());
  for(int poiIdx=0; poiIdx<getNumPointsOfInterest(); poiIdx++){
    pois[poiIdx].id = getPOIOSMNodeID(poiIdx);
    pois[poiIdx].type = getPOIType(poiIdx);
    pois[poiIdx].name = getPOIName(poiIdx);

    pois[poiIdx].LatLon_loc = getPOIPosition(poiIdx);
    float x = positionX(pois[poiIdx].LatLon_loc.longitude());
    float y = positionY(pois[poiIdx].LatLon_loc.latitude());
    pois[poiIdx].xy_loc = ezgl::point2d(x,y);


    if(pois[poiIdx].type == "college" || pois[poiIdx].type == "school" || pois[poiIdx].type == "university"){
      school_pois.push_back(pois[poiIdx]);
    } 
    if( pois[poiIdx].type == "hospital"){
      emergency_pois.push_back(pois[poiIdx]);
    } 
    if(pois[poiIdx].type == "alkohol" || pois[poiIdx].type == "bar" || pois[poiIdx].type == "biergarten" || 
    pois[poiIdx].type == "nightclub" || pois[poiIdx].type == "pub" || pois[poiIdx].type == "wine" || 
    pois[poiIdx].type == "adult_gaming_centre" || pois[poiIdx].type == "amusement_arcade"){
      leisure_pois.push_back(pois[poiIdx]);
    } 
    if(pois[poiIdx].type == "bbq" || pois[poiIdx].type == "biergarten" || 
    pois[poiIdx].type == "cafe" || pois[poiIdx].type == "fast_food" || pois[poiIdx].type == "food_court" || 
    pois[poiIdx].type == "ice_cream" || pois[poiIdx].type == "pub" || pois[poiIdx].type == "restaurant"){
      food_pois.push_back(pois[poiIdx]);
    } 
    if(pois[poiIdx].type == "charging_station" || pois[poiIdx].type == "ev_charging" || 
        pois[poiIdx].type == "fuel"){
      driving_pois.push_back(pois[poiIdx]);
    }
  }
}

//loads all features
void load_features(){
    features.resize(getNumFeatures());
    for(int featIdx=0; featIdx< getNumFeatures(); featIdx++){
       
        features[featIdx].id = getFeatureOSMID(featIdx);
        features[featIdx].type = getFeatureType(featIdx);
        features[featIdx].num_feat_points = getNumFeaturePoints(featIdx);
        features[featIdx].name = (getFeatureName(featIdx));
        features[featIdx].area = findFeatureArea(featIdx);
        features[featIdx].LatLon_locs.resize(features[featIdx].num_feat_points);
        features[featIdx].xy_locs.resize(features[featIdx].num_feat_points);

        for(int feat_point = 0; feat_point < features[featIdx].num_feat_points; feat_point++){
            features[featIdx].LatLon_locs[feat_point] = getFeaturePoint(featIdx, feat_point);
            float x = positionX(features[featIdx].LatLon_locs[feat_point].longitude());
            float y = positionY(features[featIdx].LatLon_locs[feat_point].latitude());
            features[featIdx].xy_locs[feat_point] = ezgl::point2d(x,y);
        }
        getCentroid(features[featIdx].xy_locs,featIdx);
    }
    std::sort(features.begin(),features.end(), [](feature_data &a, feature_data &b){ return a.area > b.area; });
}

//get centroid of features and min and max
void getCentroid(std::vector<ezgl::point2d> xy_locs,int feature_idx){
   if(xy_locs.empty()){
      return; // nothing to compute for a feature with no points
   }
   ezgl::point2d centroid;
   ezgl::point2d min=xy_locs[0];
   ezgl::point2d max=min;
   for (size_t i = 0; i < xy_locs.size(); ++i){

      std::max (xy_locs[i].x, max.x);
      std::min (xy_locs[i].x, min.x);
      std::max (xy_locs[i].y, max.y);
      std::min (xy_locs[i].y, min.y);
     
      centroid.x += xy_locs[i].x;
      centroid.y += xy_locs[i].y;
   }
   centroid.x = centroid.x/xy_locs.size();
   centroid.y = centroid.y/xy_locs.size();  
   
   features[feature_idx].centroid = centroid;
   features[feature_idx].min = min;
   features[feature_idx].max = max;

  //creates a rectangle that the feature sits in 
  ezgl::point2d origin = ezgl::point2d(features[feature_idx].min.x,features[feature_idx].min.y);
  ezgl::point2d top_right = ezgl::point2d(features[feature_idx].max.x,features[feature_idx].max.y);
  
  features[feature_idx].feature_rect = ezgl::rectangle(origin, top_right);
}

//load all images
void load_images(){
  traffic_light_icon= ezgl::renderer::load_png("libstreetmap/resources/traffic_light.png");
  stop_sign_icon= ezgl::renderer::load_png("libstreetmap/resources/stop_sign.png");
  subway_icon= ezgl::renderer::load_png("libstreetmap/resources/subway_icon.png");
  emergency_icon= ezgl::renderer::load_png("libstreetmap/resources/emergency_icon.png");
  school_icon= ezgl::renderer::load_png("libstreetmap/resources/school_icon.png");
  food_icon= ezgl::renderer::load_png("libstreetmap/resources/food_icon.png");
  leisure_icon= ezgl::renderer::load_png("libstreetmap/resources/leisure_icon.png");
  driving_icon= ezgl::renderer::load_png("libstreetmap/resources/driving_icon.png");
  destination_icon= ezgl::renderer::load_png("libstreetmap/resources/destination_icon.png");
  
}

//frees the images when loading map
void close_images(){
  ezgl::renderer::free_surface(traffic_light_icon);
  ezgl::renderer::free_surface(stop_sign_icon);
  ezgl::renderer::free_surface(subway_icon);
  ezgl::renderer::free_surface(emergency_icon);
  ezgl::renderer::free_surface(school_icon);
  ezgl::renderer::free_surface(food_icon);
  ezgl::renderer::free_surface(leisure_icon);
  ezgl::renderer::free_surface(driving_icon);
  ezgl::renderer::free_surface(destination_icon);
}


