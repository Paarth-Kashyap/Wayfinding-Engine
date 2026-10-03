/*This header file will include all the header files, global structs, 
and such public data types to be shared with the entire mapper program*/


#ifndef GLOBAL_H
#define GLOBAL_H

#include "m1.h"
#include "m2.h"
#include "m3.h"
#include "m4.h"


#include "ezgl/graphics.hpp"
#include "ezgl/application.hpp"
#include <color.hpp>

#include <cmath>
#include <iostream>
#include "StreetsDatabaseAPI.h"
#include "OSMDatabaseAPI.h"
#include <thread>
#include <sstream>
#include <chrono>
#include <string>
#include <set>
#include <queue>
#include <unordered_set>

#include <filesystem>

#include <list>  

#define NO_EDGE -1 //used to represent no edge (illegeal edge id)
#define INF 9999999 //used to represent infinity



extern std::vector<IntersectionIdx> destinations_test;

/* -----------------Structs--------------------- */
struct POI_data {
    OSMID id; // id of the OSM node the POI comes from
    std::string type; // General type of POI
    std::string name; // Name of POI 
    LatLon LatLon_loc; // location in LatLon
    ezgl::point2d xy_loc; // location in xy
    bool highlight = false;
    bool show = false;
};
//vectors with all different types of POIs and their data
extern std::vector<POI_data> pois; 
extern std::vector<POI_data> school_pois; 
extern std::vector<POI_data> food_pois; 
extern std::vector<POI_data> emergency_pois; 
extern std::vector<POI_data> leisure_pois; 
extern std::vector<POI_data> driving_pois; 

//struct to store the data of a feature
struct feature_data {
    TypedOSMID id;
    FeatureType type;
    std::string name;
    int num_feat_points;
    double area=0;
    double length=0;
    bool show = true;
    ezgl::point2d centroid;
    ezgl::point2d min, max;
    ezgl::rectangle feature_rect;
    std::vector<LatLon> LatLon_locs;
    std::vector<ezgl::point2d> xy_locs; // location in xy

};
extern std::vector<feature_data> features; //vector with all POIs and their data


// Road classification, resolved once at load time so the draw loop never has
// to query the OSM API or compare road-type strings per frame.
enum class RoadClass {
    OTHER = 0,      // residential / pedestrian / unclassified / service
    TERTIARY,       // tertiary / tertiary_link
    SECONDARY,      // secondary / secondary_link
    PRIMARY,        // primary / primary_link
    LINK,           // trunk_link / motorway_link
    HIGHWAY,        // motorway / trunk
    NONE            // not a drawable road
};

//struct to store data of all segmants
struct seg_data {
    LatLon start; //start of the segmant
    LatLon end; //end of the segmant
    ezgl::point2d start_xy; //cached xy of start (projected once at load)
    ezgl::point2d end_xy;   //cached xy of end (projected once at load)
    double length; //length of the segmant
    double speed; //speed of the segmant
    double travel_time; //travel time of the segmant
    double width=1.0;
    int id; //id of the segmant
    int street_id; //id of the street the segmant is in
    RoadClass road_class = RoadClass::NONE; //resolved road class (set at load)
    bool one_way=false; //if the segmant is one way
    bool highlight = false; //if the segmant is highlighted
    bool draw=false;
    std::string name; //name of the segmant
    std::vector<LatLon> curve_points; //curve points of the segmant
    std::vector<ezgl::point2d> xy_curve_points; //curve points of the segmant in xy
    ezgl::rectangle seg_rect;

    
};
extern std::vector<seg_data> segments; //vector with all segmants and their data


//struct to store the data of an intersection 
struct intersection_data {
    LatLon LatLon_loc; // location in LatLon
    ezgl::point2d xy_loc; // location in xy
    std::string name;
    int id; //id of the intersection
    bool highlight = false;
    std::vector<StreetSegmentIdx> street_segs; //holds all street segments attached to the intersection

    bool visited = false;
    StreetSegmentIdx reachingEdge; //id of edge used to reach this node
    double bestTime=INF; //best time from source to this node
};
extern std::vector<intersection_data> intersections; //vector with all intersections and their data

//struct to store the data of a street
struct street_data {
    std::string name; //name of the street
    int id; //id of the street
    double length; //length of the street
    std::vector<IntersectionIdx> intersections; //holds all intersections in the street
    std::vector<StreetSegmentIdx> street_segs; //holds all street segments in the street

};
extern std::vector<street_data> streets_struc_vec; //vector with all streets and their data

//struct to store the data of osm stuff
struct osm_data {
    OSMID id;    
    int tag_count;
    const OSMNode* node;
    LatLon LatLon_loc;
    ezgl::point2d xy_loc;
    bool show = true;
};
//vectors with different osm data
extern std::vector<osm_data> osmSubwayStations;
extern std::vector<osm_data> osmTrafficLights;
extern std::vector<osm_data> osmStopSigns;

struct waveElemAStar {
    int nodeId;
    int edgeId; //id of edge used to reach this node
    double travel_time; //total time from source to this node
    double total_est_time; //total estimated time from source to destination   
    
    waveElemAStar(int nId, int eId, double t_time, double est_time){
        this->nodeId = nId;
        this->edgeId = eId;
        this->travel_time = t_time;
        this->total_est_time = est_time;
    }
    // Comparison operator for priority queue
    bool operator>(const waveElemAStar& other) const {
        // Compare by total estimated time
        return total_est_time > other.total_est_time;
    }
};



/* Data Structures */
extern std::vector<double> streets_length;  // holds length of each street (the location of length represents streetIdx)
extern std::vector<double> street_seg_travel_time; //vector with all street segments' travel times [StreetSegmentIdx][travel_time of seg]
extern std::vector<StreetSegmentIdx> street_segs;  // vector with all streetSegmants
extern std::vector<std::vector<StreetSegmentIdx>> streets;  // holds [street][street_seg of that street]
extern std::vector<std::vector<StreetSegmentIdx>> intersection_street_segments; //holds intersections and thier segments 
extern std::vector<std::vector<IntersectionIdx>> street_intersections; //vector with all streets and their intersections [street][intersection in street]
extern std::unordered_map<OSMID, const OSMWay*> osmWayMap; //stores OSMWay objects based on OSMID
extern std::unordered_map<OSMID, const OSMNode*> osmNodeMap; //stores OSMNode objects based on OSMID
extern std::unordered_map<std::string, std::vector<StreetIdx>> prefixMap;
extern std::unordered_map<OSMID, std::vector<std::pair<std::string, std::string>>> osmNode_pairs;
extern std::unordered_map<OSMID, std::string> osmRoadMap;
//extern std::unordered_map<OSMID, std::string> osmSubwayRoutes;

/*------------------global variables-----------------------------*/
extern double oneWayAngleCoef; 
extern double avg_lat;
extern double avg_lat_cos; //cos(avg_lat*kDegreeToRadian)
extern double max_lat; 
extern double min_lat; 
extern double max_lon; 
extern double min_lon; 
extern double max_x;
extern double min_x;
extern double max_y;
extern double min_y;
extern double max_speed_limit; 
extern std::vector<StreetSegmentIdx> my_path;
extern std::pair<IntersectionIdx, IntersectionIdx> my_path_start_end;


//image variables
extern ezgl::surface* traffic_light_icon;
extern ezgl::surface* stop_sign_icon;
extern ezgl::surface* subway_icon;
extern ezgl::surface* emergency_icon;
extern ezgl::surface* school_icon;
extern ezgl::surface* food_icon;
extern ezgl::surface* leisure_icon;
extern ezgl::surface* driving_icon;
extern ezgl::surface* destination_icon;

/*-------------------Global functions--------------------------*/
double positionX(double);
double positionY(double);
double positionLat(double);
double positionLon(double);


/* Variables and Data Structures */

// std::unordered_map<IntersectionIdx, bool> visitedHash; //store the visited intersections

//delay function that assists in the visualization of the algorithm
//used in testing
//takes in an integer value that represents the number of milliseconds to delay
void visualDelay(int);

//draws the highlighting of the algorithm exploring different street segments
//used for testing
//takes in the renderer, the street segment index and the color to highlight the street segment
void highlightStreetSegment(ezgl::renderer*, StreetSegmentIdx, ezgl::color);


//calculates the total time to get to current wave element
//takes in the turn penalty, the wave element and the next street segment index
//used in the A* algorithm
double calcTotalTime(double, waveElemAStar, StreetSegmentIdx);


//calculates the aerial distance to reach the destination intersection from current intersection
//takes in the current intersection index and the destination intersection index
//used in the A* algorithm
double getAerialTravelTime(IntersectionIdx, IntersectionIdx);


//A* path finding algorithm
//takes in the source intersection index, the destination intersection index and the turn penalty
//returns a boolean value indicating if a path is found or not
bool astarPath(IntersectionIdx, IntersectionIdx, const double);
void dijkstraExpansion(float turn_penalty, IntersectionIdx,const std::unordered_set<IntersectionIdx>&);

/* ---- A* search visualization ----
   When search_vis_enabled is true, astarPath draws each edge it explores onto
   search_vis_renderer and flushes the frame, so the user can watch the
   wavefront expand before the final path is shown. The UI sets these before
   invoking a route and clears them afterwards. All null/false by default so
   routing stays fast unless visualization is explicitly turned on. */
extern bool                search_vis_enabled;
extern ezgl::application*  search_vis_app;
extern ezgl::renderer*     search_vis_renderer;
extern int                 search_vis_delay_ms; // per-step delay in milliseconds


//BFS trace back algorithm
//takes in the source intersection index and the destination intersection index
//returns a vector of street segment indices that represent the path
std::vector<StreetSegmentIdx> bfsTraceBack(IntersectionIdx);
std::vector<StreetSegmentIdx> bfsTraceBack(IntersectionIdx dest,const std::unordered_map<IntersectionIdx, intersection_data> &);


void displaySeg(const seg_data&, ezgl::renderer*,ezgl::color, double);

/* m1 query functions not declared in the public milestone header */
double findAngleBetweenStreetSegments(StreetSegmentIdx src_street_segment_id, StreetSegmentIdx dst_street_segment_id);
bool intersectionsAreDirectlyConnected(std::pair<IntersectionIdx, IntersectionIdx> intersection_ids);

/* functions used in m1 Helper Function Declaration */
RoadClass classifyRoad(const std::string& road_type);
void load_intersection_street_segs();
void load_Street_and_Segment();
void load_streets_length();
void load_OSMWayMap();
void load_OSMNodeMap();
void load_OSMRelations();
void load_street_seg_travel_time();
void load_street_intersections();
void loadLatLonStuff();
void getCentroid(std::vector<ezgl::point2d> ,int);
void load_POIs();
void load_features();
void load_images();
void close_images();
void load_PrefixMap();
double FindAngle(double, double, double, double, double, double);


std::vector<POIIdx> load_POI(std::string);
std::vector<LatLon> load_feature(FeatureIdx);  
std::string compressedStreetName(const std::string& name);

/*functions used in m2 helper*/
bool intersects(ezgl::rectangle, ezgl::rectangle);


/*---------------- Colours ------------------------*/
static constexpr ezgl::color LIGHT_GREEN(195, 241, 213);
static constexpr ezgl::color DARK_GREEN(28, 58, 34);
static constexpr ezgl::color LIGHT_BLUE(144, 218, 238);
static constexpr ezgl::color SAND(247, 236, 207);
static constexpr ezgl::color HOUSE_COLOR(141, 134, 203);
static constexpr ezgl::color HIGHLIGHTED_COL(15, 83, 255);

static constexpr ezgl::color DARK_GREY(139, 165, 193,255);
static constexpr ezgl::color GREY(204, 202, 196);
static constexpr ezgl::color LIGHT_GREY(216, 224, 231,255);
static constexpr ezgl::color LIGHTER_GREY(235, 242, 247);

static constexpr ezgl::color DM_BACKGROUND(43, 43, 43);
static constexpr ezgl::color DM_LAKE(17, 51, 89);
static constexpr ezgl::color DM_RIVER(43, 76, 127);
static constexpr ezgl::color DM_SAND(153, 124, 90);
static constexpr ezgl::color DM_BUILDINGS(73, 74, 75);
static constexpr ezgl::color DM_LARGE_STREETS(143, 153, 110);
static constexpr ezgl::color DM_SMALL_STREETS(138, 128, 130);
static constexpr ezgl::color DM_HIGHLIGHT_COL(0, 158, 207);
static constexpr ezgl::color DM_STREET_NAME(255, 255, 255);



//m4 stuff
extern std::vector<std::vector<double>> travelTime_matrix; //[src][dest]-->holds the travel time from src to dest

//create a hash table to store a the travel time from one intersection to another and its path
extern std::unordered_map<IntersectionIdx, std::multimap<IntersectionIdx, std::pair<double,std::vector<StreetSegmentIdx>>>> travelMatrix_map;
void load_travelTime_Matrix(const std::vector<IntersectionIdx>&, const std::unordered_set<IntersectionIdx>& ,float);


#endif /* GLOBAL_H */