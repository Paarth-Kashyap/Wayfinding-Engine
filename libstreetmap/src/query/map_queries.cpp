#include "global.h"

/* map_queries.cpp
   Query / geometry / projection code moved from m1.cpp (no behavior change). */


/* Returns the distance between two (lattitude,longitude) coordinates in meters.
Speed Requirement --> moderate */
double findDistanceBetweenTwoPoints(std::pair<LatLon, LatLon> points) {
  LatLon point_1 = points.first;
  LatLon point_2 = points.second;
  double lat1 = kDegreeToRadian * point_1.latitude();
  double lat2 = kDegreeToRadian * point_2.latitude();
  double lon1 = kDegreeToRadian * point_1.longitude();
  double lon2 = kDegreeToRadian * point_2.longitude();

  double lat_avg = (lat1 + lat2) / 2.0;

  double x1 = kEarthRadiusInMeters * lon1 * cos(lat_avg);
  double x2 = kEarthRadiusInMeters * lon2 * cos(lat_avg);

  double y1 = kEarthRadiusInMeters * lat1;
  double y2 = kEarthRadiusInMeters * lat2;

  double distance = sqrt(pow((y2 - y1),2.0) + pow((x2 - x1),2.0));
  return distance;
}

/* Returns the length of the given street segment in meters.
Speed Requirement --> moderate */
double findStreetSegmentLength(StreetSegmentIdx street_segment_id) {
  
  StreetSegmentInfo street_seg_info = getStreetSegmentInfo(street_segment_id);

  // Constructs previous and next position when traversing through the street segments
  LatLon previous_position = getIntersectionPosition(street_seg_info.from);
  LatLon next_position;

  int num_curve_points = street_seg_info.numCurvePoints;
  double street_seg_length = 0.0;

  // Traverses through number of curve points
  for (int i = 0; i < num_curve_points; i++) { 
    next_position = getStreetSegmentCurvePoint(street_segment_id, i);
    street_seg_length += findDistanceBetweenTwoPoints(std::make_pair(previous_position, next_position));
    previous_position = next_position; // Updates previous position
  }

  // From the last position to street_seg_info.to, for case where there are no curve points, just a straight line
  street_seg_length += findDistanceBetweenTwoPoints(std::make_pair(previous_position, getIntersectionPosition(street_seg_info.to)));

  return street_seg_length;
}

/* Returns the travel time to drive from one end of a street segment
  to the other, in seconds, when driving at the speed limit.
  Note: (time = distance/speed_limit)
Speed Requirement --> high */
double findStreetSegmentTravelTime(StreetSegmentIdx street_segment_id) {
  return street_seg_travel_time[street_segment_id];
}

/* Returns the angle (in radians) that would result as you exit
  src_street_segment_id and enter dst_street_segment_id, if they share an
  intersection.
  If a street segment is not completely straight, use the last piece of the
  segment closest to the shared intersection.
  If the two street segments do not share an intersection, return a constant
  NO_ANGLE, which is defined above.
Speed Requirement --> none */
double findAngleBetweenStreetSegments(StreetSegmentIdx src_street_segment_id, StreetSegmentIdx dst_street_segment_id) {
    // Checking first if the two street segments share an intersection
    // 1 is source, 2 is destination
    StreetSegmentInfo street_seg_info_1 = getStreetSegmentInfo(src_street_segment_id);
    StreetSegmentInfo street_seg_info_2 = getStreetSegmentInfo(dst_street_segment_id);

    // Check all four possibilities of sharing an intersection
    IntersectionIdx shared_intersection;
    int possibility = 0;

    // Updates the shared intersection and possibility depending on the segment intersection orientation
    // Checks if segments meet end-to-end
    if (street_seg_info_1.to == street_seg_info_2.to) {
        possibility = 1;
        shared_intersection = street_seg_info_1.to;
    // Checks if segments meet start-to-end
    } else if (street_seg_info_1.from == street_seg_info_2.to) {
        possibility = 2;
        shared_intersection = street_seg_info_1.from;
    // Checks if segments meet end-to-start
    } else if (street_seg_info_1.to == street_seg_info_2.from) {
        possibility = 3;
        shared_intersection = street_seg_info_1.to;
    // Checks if segments meet start-to-start
    } else if (street_seg_info_1.from == street_seg_info_2.from) {
        possibility = 4;
        shared_intersection = street_seg_info_1.from;
    }

    // If no shared intersection, we return NO_ANGLE
    if (possibility == 0) {  
        return NO_ANGLE;
    }

    // Assumed that there is a shared intersection beyond this point
    LatLon point1;
    LatLon point2;

    // Updates point 1 and point 2 depending on segment intersection orientation
    switch (possibility) {
    // If segments meet end-to-end
    case 1:
      if (street_seg_info_1.numCurvePoints != 0) {
        point1 = getStreetSegmentCurvePoint(src_street_segment_id, street_seg_info_1.numCurvePoints - 1);
      } else {
        point1 = getIntersectionPosition(street_seg_info_1.from);
      }
      if (street_seg_info_2.numCurvePoints != 0) {
        point2 = getStreetSegmentCurvePoint(
            dst_street_segment_id, street_seg_info_2.numCurvePoints - 1);
      } else {
        point2 = getIntersectionPosition(street_seg_info_2.from);
      }
      break;
    // If segments meet start-to-end
    case 2:
      if (street_seg_info_1.numCurvePoints != 0) {
        point1 = getStreetSegmentCurvePoint(src_street_segment_id, 0);
      } else {
        point1 = getIntersectionPosition(street_seg_info_1.to);
      }
      if (street_seg_info_2.numCurvePoints != 0) {
        point2 = getStreetSegmentCurvePoint(
            dst_street_segment_id, street_seg_info_2.numCurvePoints - 1);
      } else {
        point2 = getIntersectionPosition(street_seg_info_2.from);
      }
      break;
    // If segments meet end-to-start
    case 3:
      if (street_seg_info_1.numCurvePoints != 0) {
        point1 = getStreetSegmentCurvePoint(
            src_street_segment_id, street_seg_info_1.numCurvePoints - 1);
      } else {
        point1 = getIntersectionPosition(street_seg_info_1.from);
      }
      if (street_seg_info_2.numCurvePoints != 0) {
        point2 = getStreetSegmentCurvePoint(dst_street_segment_id, 0);
      } else {
        point2 = getIntersectionPosition(street_seg_info_2.to);
      }
      break;
    // If segments meet start-to-start
    case 4:
      if (street_seg_info_1.numCurvePoints != 0) {
        point1 = getStreetSegmentCurvePoint(src_street_segment_id, 0);
      } else {
        point1 = getIntersectionPosition(street_seg_info_1.to);
      }
      if (street_seg_info_2.numCurvePoints != 0) {
        point2 = getStreetSegmentCurvePoint(dst_street_segment_id, 0);
      } else {
        point2 = getIntersectionPosition(street_seg_info_2.to);
      }
      break;
    // Should never be reached unless output is an error
    default:
      std::cout << "ERROR: Reached default case in switch of function findAngleBetweenStreetSegments" << std::endl;
      break;
  }

  LatLon shared_inter_point = getIntersectionPosition(shared_intersection);

  // We need to convert the LatLon values into (x, y) coordinate points
  double lat1 = (M_PI/180.0) * point1.latitude();
  double lon1 = (M_PI/180.0) * point1.longitude();

  double lat2 = (M_PI/180.0) * point2.latitude();
  double lon2 = (M_PI/180.0) * point2.longitude();

  double lat_intersection = (M_PI/180.0) * shared_inter_point.latitude();
  double lon_intersection = (M_PI/180.0) * shared_inter_point.longitude();

  double lat_avg = ((std::min(lat1, std::min(lat2, lat_intersection)) + std::max(lat1, std::max(lat2, lat_intersection)))/2);

  double x1 = kEarthRadiusInMeters * lon1 * cos(lat_avg);
  double x2 = kEarthRadiusInMeters * lon2 * cos(lat_avg);
  double x_intersection = kEarthRadiusInMeters * lon_intersection * cos(lat_avg);

  double y1 = kEarthRadiusInMeters * lat1;
  double y2 = kEarthRadiusInMeters * lat2;
  double y_intersection = kEarthRadiusInMeters * lat_intersection;

  double angle = FindAngle(x1, y1, x2, y2, x_intersection, y_intersection);

  return angle;
}

/* Returns true if the two intersections are directly connected, meaning you can
  legally drive from the first intersection to the second using only one
  streetSegment.
Speed Requirement --> moderate */
bool intersectionsAreDirectlyConnected(std::pair<IntersectionIdx, IntersectionIdx> intersection_ids) {

  // Load all street segments for the two intersections
  std::vector<StreetSegmentIdx> src_intersec = findStreetSegmentsOfIntersection(intersection_ids.first);
  std::vector<StreetSegmentIdx> dest_intersec = findStreetSegmentsOfIntersection(intersection_ids.second);

  // Edge cases for when the two intersections are the same
  if (intersection_ids.first == intersection_ids.second) {
    return true;
  }
  
  // Loops through all street segments of the first intersection
  for (int i = 0; i < src_intersec.size(); i++) {
    // Loops through all street segments of the second intersection
    for (int j = 0; j < dest_intersec.size(); j++) {

      // If the two street segments are the same, return true
      if (src_intersec[i] == dest_intersec[j]) {
        // If one way, check if it can travel from first intersection to second intersection
        if(getStreetSegmentInfo(src_intersec[i]).oneWay){
          if (getStreetSegmentInfo(src_intersec[i]).to == intersection_ids.second) {
            return true;
          }
          // Possible to travel both ways
        }else{
          return true;
        }
      }
    }
  }
  return false;
}


/* Returns the geographically nearest intersection 
  (i.e. as the crow flies) to the given position.
Speed Requirement --> none */
IntersectionIdx findClosestIntersection(LatLon my_position){
  IntersectionIdx closest = 0;
  double currentdistance = 10000000000;

  // Iterates through each intersection and checks distance between that and my_position
  for(int i = 0; i < getNumIntersections(); i++){
    LatLon interposition = getIntersectionPosition(i);

    double distance = findDistanceBetweenTwoPoints(std::make_pair(my_position, interposition));

    // When a distance is found to be smaller, update closest
    if(distance < currentdistance){
        currentdistance = distance;
        closest = i;
    }
  }
  return closest;
}

/* Returns the street segments that connect to the given intersection.
  Speed Requirement --> high */
std::vector<StreetSegmentIdx> findStreetSegmentsOfIntersection(
	IntersectionIdx intersection_id) {
    return intersection_street_segments[intersection_id];
}

/* Returns all intersections along the given street.
  There should be no duplicate intersections in the returned vector.
Speed Requirement --> high */
std::vector<IntersectionIdx> findIntersectionsOfStreet(StreetIdx street_id) {
  return street_intersections[street_id];
}

/* Return all intersection ids at which the two given streets intersect.
  This function will typically return one intersection id for streets that
  intersect and a length 0 vector for streets that do not. For unusual curved
  streets it is possible to have more than one intersection at which two
  streets cross.
  There should be no duplicate intersections in the returned vector.
Speed Requirement --> high */
std::vector<IntersectionIdx> findIntersectionsOfTwoStreets(
	StreetIdx street_id1, StreetIdx street_id2) {

  // Uses findIntersectionsOfStreet to identify both street ids
  std::vector<IntersectionIdx> correct;
  std::vector<IntersectionIdx> intersectionone = findIntersectionsOfStreet(street_id1);
  std::vector<IntersectionIdx> intersectiontwo = findIntersectionsOfStreet(street_id2);

  // Loops through both street_ids until a match is found
  for(int i = 0; i < intersectionone.size(); i++){
    for(int j = 0; j < intersectiontwo.size(); j++){
      if(intersectionone[i] == intersectiontwo[j]){
        correct.emplace_back(intersectionone[i]);
      }
    }
  }
    return correct;
}

/* Returns all street ids corresponding to street names that start with the
  given prefix.
  The function should be case-insensitive to the street prefix.
  The function should ignore spaces.
  For example, both "bloor " and "BloOrst" are prefixes to
    "Bloor Street East".
  If no street names match the given prefix, this routine returns an empty
  (length 0) vector.
  You can choose what to return if the street prefix passed in is an empty
  (length 0) string, but your program must not crash if street_prefix is a
  length 0 string.
Speed Requirement --> high */
std::vector<StreetIdx> findStreetIdsFromPartialStreetName(std::string street_prefix) {
  std::string compressedPrefix = compressedStreetName(street_prefix); // Removes formatting for input street_prefix
  return prefixMap[compressedPrefix];
}

/* Returns the length of a given street in meters.
Speed Requirement --> high */
double findStreetLength(StreetIdx street_id) {
	return streets_length[street_id];
}

/* Returns the nearest point of interest of the given name (e.g. "Starbucks")
  to the given position.
Speed Requirement --> none */
POIIdx findClosestPOI(LatLon my_position, std::string poi_name) {

  // Create vector to hold all POIs with the given name
  std::vector<POIIdx> POIs = load_POI(poi_name);
  POIIdx closest = -1;
  double currentdistance;

  // Find the LATLON for the POIs
  for (int poiIdx = 0; poiIdx < POIs.size(); poiIdx++) {
    LatLon POI_position = getPOIPosition(POIs[poiIdx]);
    double distance = findDistanceBetweenTwoPoints(std::make_pair(my_position, POI_position));
    
    // Set closets POI
    if(poiIdx==0){
      currentdistance = distance;
      closest = POIs[poiIdx];
    }
    else if (distance < currentdistance) {
      currentdistance = distance;
      closest = POIs[poiIdx];
    }
  }
  return closest;
}

/* Returns the area of the given closed feature in square meters.
  Assume a non self-intersecting polygon (i.e. no holes).
  Return 0 if this feature is not a closed polygon.
Speed Requirement --> moderate */
double findFeatureArea(FeatureIdx feature_id) {
  
  // Get all points for a feature 
  std::vector<LatLon> feature_points =load_feature(feature_id);
  double totalArea=0;

  // A feature with no points cannot have an area
  if(feature_points.empty()){
    return 0.0;
  }

  // Check if the feature is a closed polygon
  if(feature_points[0].latitude() == feature_points[feature_points.size()-1].latitude() && 
  feature_points[0].longitude() == feature_points[feature_points.size()-1].longitude()){
    double x1, x2, y1, y2, xavg, ydiff;
    double lat1, lat2, lon1, lon2;
    double lat_avg;
    int m=0;
    // Calculate the area of the feature
    for (int i = 0; i < feature_points.size(); i++) {
      
      // Convert LonLat to x,y
      // Index edge case
      if(i<feature_points.size()-1){m=i+1;
      } else { m=0; }
      // Convert to radians
      lat1 = kDegreeToRadian* feature_points[i].latitude();
      lon1 = kDegreeToRadian* feature_points[i].longitude();
      lat2 = kDegreeToRadian * feature_points[m].latitude();
      lon2 = kDegreeToRadian * feature_points[m].longitude();
           
      lat_avg = (lat1+lat2)/2.0;
      x1 = kEarthRadiusInMeters * lon1 * cos(lat_avg);
      y1 = kEarthRadiusInMeters * lat1;
      x2 = kEarthRadiusInMeters * lon2 * cos(lat_avg);
      y2 = kEarthRadiusInMeters * lat2;

      // Calculate area
      xavg= (x1 + x2) / 2.0;
      ydiff = y2 - y1;
      totalArea += xavg*ydiff;
    }
   return std::abs(totalArea); // Return the absolute value of the area
  } else {
    return 0.0; // Feature area is not a closed polygon
  }
}

/* Returns the length of the OSMWay that has the given OSMID, in meters.
  To implement this function you will have to  access the OSMDatabaseAPI.h
  functions.
Speed Requirement --> high */
double findWayLength(OSMID way_id) {

  // Find the OSMWay nodes based on the way_id 
  const std::vector<OSMID>& wayNodes = getWayMembers(osmWayMap[way_id]);
  double totalLength = 0.0;
  
  // If the way has only one node, return 0
  if(wayNodes.size() <= 1){
    return 0.0;
  }

  // Loop through all nodes and calculate the distance between each node
  for (auto i = 0; i < wayNodes.size()-1; ++i) {
    LatLon node1 = getNodeCoords(osmNodeMap[wayNodes[i]]);
    LatLon node2 = getNodeCoords(osmNodeMap[wayNodes[i+1]]);
    totalLength += findDistanceBetweenTwoPoints(std::make_pair(node1, node2));
  }

  // If way is closed, calculate the distance between the first and last node
  if (isClosedWay(osmWayMap[way_id])){
    LatLon node1 = getNodeCoords(osmNodeMap[wayNodes[0]]);
    LatLon node2 = getNodeCoords(osmNodeMap[wayNodes[wayNodes.size()-1]]);
    totalLength += findDistanceBetweenTwoPoints(std::make_pair(node1, node2));
  }

  return totalLength;   
}

/* Return the value associated with this key on the specified OSMNode.
  If this OSMNode does not exist in the current map, or the specified key is
  not set on the specified OSMNode, return an empty string.
Speed Requirement --> high */
std::string getOSMNodeTagValue(OSMID osm_id, std::string key) {
  // Access the node based on OSMID, iterate through the tags of node
  for (int tagIdx=0; tagIdx<getTagCount(osmNodeMap[osm_id]); tagIdx++){
    std::pair<std::string, std::string> tag = getTagPair(osmNodeMap[osm_id], tagIdx);
    // If match, return value
    if (tag.first == key){
      return tag.second;
    }
  }
	return "";
}

/* loads all POI based on name in vector
  returns vector with match names of POIs */
std::vector<POIIdx> load_POI(std::string poi_name) {
  int numPOI = getNumPointsOfInterest();
  std::vector<POIIdx> POIs;  // Vector with all POIs based on name
  // Loop through all POIs
  for (int poiIdx = 0; poiIdx < numPOI; poiIdx++) {
    std::string POI_name = getPOIName(poiIdx);
    // If the POI name matches the name being searched, add to vector
    if (POI_name == poi_name) {
      POIs.push_back(poiIdx);
    }
  }
  return POIs; // Return the vector of matching POIs
}

/* loads vector with the all points in that feature */
std::vector<LatLon> load_feature(FeatureIdx feature_id) {
  std::vector<LatLon> features_points_loc;
  // Get number of points for given feature
  int numPoints = getNumFeaturePoints(feature_id);
  for (int pointIdx = 0; pointIdx < numPoints; pointIdx++) {
    // Add all points to vector
    features_points_loc.push_back(getFeaturePoint(feature_id, pointIdx));
  }
  return features_points_loc;
}
/* compresses the street name to match prefix in input */
std::string compressedStreetName(const std::string& name){
  std::string compressed;
  // Loop through all street names to remove capitals and spaces
  for (int each = 0; each < name.length(); each++){
    char street = name[each];
      if (std::isalpha(street)){
        compressed += std::tolower(street);
      }
    }
  return compressed;
}
/* returns a double that is the angle between two points that share an intersection */
double FindAngle(double x1, double y1, double x2, double y2, double x_intersection, double y_intersection) {
    double line1x = x1 - x_intersection;
    double line1y = y1 - y_intersection;
    double line2x = x2 - x_intersection;
    double line2y = y2 - y_intersection;

    double line1dotline2 = line1x*line2x + line1y*line2y; // Computes the dot product between both lines

    double magnitudeLine1 = sqrt(line1x*line1x + line1y*line1y);
    double magnitudeLine2 = sqrt(line2x*line2x + line2y*line2y);

    double cosTheta = line1dotline2/(magnitudeLine1*magnitudeLine2);

    return (M_PI - acos(cosTheta)); // Returns the angle between two points
}
//final submission
//Longitude to X
double positionX(double lon){
  return( kEarthRadiusInMeters * kDegreeToRadian * lon* avg_lat_cos);
}
//Latitude to Y
double positionY(double lat){
   return(kEarthRadiusInMeters * kDegreeToRadian* lat);
}
//Y to latitude
double positionLat(double y){
  return(y / (kEarthRadiusInMeters * kDegreeToRadian));
}
//X to longitude
double positionLon(double x){
  return( x / (kEarthRadiusInMeters * kDegreeToRadian * avg_lat_cos));
}

