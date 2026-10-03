#include "global.h"


// Returns the time required to travel along the path specified, in seconds.
// The path is given as a vector of street segment ids, and this function can
// assume the vector either forms a legal path or has size == 0.  The travel
// time is the sum of the length/speed-limit of each street segment, plus the
// given turn_penalty (in seconds) per turn implied by the path.  If there is
// no turn, then there is no penalty. Note that whenever the street id changes
// (e.g. going from Bloor Street West to Bloor Street East) we have a turn.
double computePathTravelTime(const double turn_penalty, const std::vector<StreetSegmentIdx>& path) {
    double travel_time = 0.0;
    
    // Check if the path has at least two segments
    if (path.size() < 2) {
        return travel_time;
    }

    for(int i=0; i<(path.size()); i++){

        //if not the last segment
        if(i!=path.size()-1){
            StreetIdx streetID_current=getStreetSegmentInfo(path[i]).streetID;
            StreetIdx streetID_next=getStreetSegmentInfo(path[i+1]).streetID;
           
            if(streetID_current != streetID_next){
                travel_time += turn_penalty;
            }
        }
        travel_time += findStreetSegmentTravelTime(path[i]);
    }
    return travel_time;
}

// Returns a path (route) between the start intersection (intersect_id.first)
// and the destination intersection (intersect_id.second), if one exists. 
// This routine should return the shortest path
// between the given intersections, where the time penalty to turn right or
// left is given by turn_penalty (in seconds).  If no path exists, this routine
// returns an empty (size == 0) vector.  If more than one path exists, the path
// with the shortest travel time is returned. The path is returned as a vector
// of street segment ids; traversing these street segments, in the returned
// order, would take one from the start to the destination intersection.
std::vector<StreetSegmentIdx> findPathBetweenIntersections(const double turn_penalty, const std::pair<IntersectionIdx, IntersectionIdx> intersect_ids){
    // Trivial case: start == destination -> empty path.
    if(intersect_ids.first == intersect_ids.second){
        return {};
    }

    bool pathFound = astarPath(intersect_ids.first, intersect_ids.second, turn_penalty);
    if(pathFound){
        return bfsTraceBack(intersect_ids.second);
    }
    return {}; // no path exists
}


//returns the shortest path between the 2 intersections
std::vector<StreetSegmentIdx> bfsTraceBack(IntersectionIdx dest){
    std::vector<StreetSegmentIdx> path;
    //define the destination intersection
    IntersectionIdx curr = dest;
    StreetSegmentIdx prevEdge= intersections[curr].reachingEdge; //get the street segment used to reach the destination intersection

    while(prevEdge != NO_EDGE){

        path.push_back(prevEdge); //add the street segment to the path

        if(segments[prevEdge].one_way){
            curr= getStreetSegmentInfo(prevEdge).from; //get the intersection at the start of the street segment
        }else{
        curr=(curr==getStreetSegmentInfo(prevEdge).from)? getStreetSegmentInfo(prevEdge).to : getStreetSegmentInfo(prevEdge).from;
        }
        
        prevEdge = intersections[curr].reachingEdge; //get the street segment used to reach the next intersection
    }
    //reverse the path
    std::reverse(path.begin(), path.end());

    return path; //return the path as vector
}

//returns true if a path is found between the 2 intersections
bool astarPath(IntersectionIdx src, IntersectionIdx dest, const double turn_penalty) {
    bool pathFound=false;

   //reset the best time for all nodes
    for(auto &id: intersections){
        id.bestTime=INF;
        id.reachingEdge=NO_EDGE;
    }

    //implement the wavefront as min heap
    std::priority_queue <waveElemAStar, std::vector<waveElemAStar>, std::greater<waveElemAStar>> minHeapWave; 
    waveElemAStar wave1(src, NO_EDGE, 0.0, getAerialTravelTime(src, dest));
    minHeapWave.push(wave1);


    // loop through until the wavefront is empty
    while (!minHeapWave.empty()){    

        waveElemAStar wave = minHeapWave.top();
        minHeapWave.pop();
        int currID=wave.nodeId;
        double currTime=wave.travel_time; 

        //checks if the travel time to current node is less than the best time to the same node
        if(currTime < intersections[currID].bestTime){

            intersections[currID].reachingEdge=wave.edgeId;
            intersections[currID].bestTime=wave.travel_time;

            // check if the current intersection is the destination afer updating its best time and reasching edge
            if (currID == dest) {
                pathFound=true;
                break;
            }
        
            for(auto &outEdge: intersections[currID].street_segs){

                //check if the street segment is one way and is not going to the current intersection
                if (segments[outEdge].one_way) {
                    if (getStreetSegmentInfo(outEdge).from != currID) {
                        continue;
                    }
                }

                if(outEdge==wave.edgeId){
                    continue;
                }


                //checks the which way to go on the street if its 2 ways
                IntersectionIdx toNode = (currID==getStreetSegmentInfo(outEdge).from)? getStreetSegmentInfo(outEdge).to : getStreetSegmentInfo(outEdge).from;

                double travelTime = calcTotalTime(turn_penalty, wave, outEdge);   //get the travel time to the next intersection
                double aerialTime = getAerialTravelTime(toNode, dest);    //get the aerial distance travel time to the destination
                minHeapWave.push(waveElemAStar(toNode, outEdge, travelTime, aerialTime + travelTime));

                // Live visualization: draw the edge being explored, flush the
                // frame, and pause briefly so the wavefront is visible.
                if (search_vis_enabled && search_vis_renderer && search_vis_app) {
                    search_vis_renderer->set_color(ezgl::RED);
                    search_vis_renderer->set_line_width(2);
                    search_vis_renderer->draw_line(segments[outEdge].start_xy,
                                                   segments[outEdge].end_xy);
                    search_vis_app->flush_drawing();
                    if (search_vis_delay_ms > 0)
                        visualDelay(search_vis_delay_ms);
                }
             }
        }
    }     
    return pathFound; // return false if no path is found
}


/*
//function used in m4, dijisktra keeps going until all intersections are found that are being looked for
bool dijkstraExpansion(IntersectionIdx src, std::vector<IntersectionIdx> destinations){
    bool pathFound=false;
    int numDest=destinations.size();
    int numFound=0;
    //create a hash map intersections sized to the number of intersections
    std::unordered_map<IntersectionIdx, intersection_data > intersectionsHash;
    
    //create a variable to store the current intersection as intersect_data type 
    intersection_data srcData=intersections[src];
    srcData.bestTime=0;
    srcData.reachingEdge=NO_EDGE;
    srcData.visited=true;
    intersectionsHash.emplace(src, srcData);

    //convert the vector of destinations into a hash set
    std::unordered_map<IntersectionIdx, intersection_data > destMap;
    for (auto id : destinations) {
        destMap.emplace(id, false); // Set all destinations as not found initially
    }


    //implement the wavefront as min heap
    std::priority_queue <waveElemAStar, std::vector<waveElemAStar>, std::greater<waveElemAStar>> minHeapWave; 
    waveElemAStar wave1(src, NO_EDGE, 0.0, 0.0);
    minHeapWave.push(wave1);


    // loop through until the wavefront is empty
    while (!minHeapWave.empty()){    

        waveElemAStar wave = minHeapWave.top();
        minHeapWave.pop();
        int currID=wave.nodeId;
        double currTime=wave.travel_time; 


        // Access intersection data from the hash map
        intersection_data& currData = intersectionsHash[currID];//-------------------

        //checks if the travel time to current node is less than the best time to the same node
        if(currTime < currData.bestTime){

            currData.reachingEdge=wave.edgeId;
            currData.bestTime=wave.travel_time;

            //add the current intersection to the hash 
            intersectionsHash[currID]=currData;



            // Check if current intersection is a destination
            intersection_data destIt = destMap[currID];
            if (destIt.id==currID ) { 
                destIt.visited=true; // Mark destination as found
                numFound++;
            }

            // Check if all destinations are found
            if (numFound == numDest) {
                pathFound = true;
                break;
            }
        
            for(auto &outEdge: findStreetSegmentsOfIntersection(currID)){

                //check if the street segment is one way and is not going to the current intersection
                if (segments[outEdge].one_way) {
                    if (getStreetSegmentInfo(outEdge).from != currID) {
                        continue;
                    }
                }
                if(outEdge==wave.edgeId){
                    continue;
                }
                //checks the which way to go on the street if its 2 ways
                IntersectionIdx toNode = (currID==getStreetSegmentInfo(outEdge).from)? getStreetSegmentInfo(outEdge).to : getStreetSegmentInfo(outEdge).from;

                double travelTime = segments[outEdge].travel_time + currData.bestTime;   //get the travel time to the next intersection
                minHeapWave.push(waveElemAStar(toNode, outEdge, travelTime, travelTime));
                // Path-finding algorithm code
                #ifdef VISUALIZE
                    // Draw edge (streetSegment) we’re about to explore
                    ezgl::renderer *g = application->get_renderer();
                    highlightStreetSegment (g, outEdge, ezgl::RED);
                    application->flush_drawing(); // Draw right now!
                    delay (5); // wait for 10 ms so we can see progress
                    visualDelay (2); // wait for 10 ms so we can see progress
                 #endif
             }
        }
    }     
    return pathFound; // return false if no path is found
}
*/

//adds turn penalty if the street changes
double calcTotalTime(double turnTime, waveElemAStar wave, StreetSegmentIdx nextID){
 
    double tTime = (double) intersections[wave.nodeId].bestTime + segments[nextID].travel_time;
   
    // check if the street changed and not at start
    if(wave.edgeId != NO_EDGE && segments[wave.edgeId].street_id != segments[nextID].street_id){
        tTime += turnTime;
    }
    return tTime;
}

//get arial distance travel time between the 2 intersections in seconds
double getAerialTravelTime(IntersectionIdx src, IntersectionIdx dest){
    double aerialDistance = findDistanceBetweenTwoPoints(std::make_pair(intersections[src].LatLon_loc, intersections[dest].LatLon_loc));
    return (double) (aerialDistance) / max_speed_limit;
}

void highlightStreetSegment(ezgl::renderer* r, StreetSegmentIdx seg, ezgl::color col){
    //get the street segment information
    StreetSegmentInfo segInfo = getStreetSegmentInfo(seg);

    //get the start and end intersection of the street segment
    IntersectionIdx start = segInfo.from;
    IntersectionIdx end = segInfo.to;

    //get the position of the start and end intersection
    LatLon start_pos = getIntersectionPosition(start);
    LatLon end_pos = getIntersectionPosition(end);

    //get the xy position of the start and end intersection
    ezgl::point2d start_xy = ezgl::point2d(positionX(start_pos.longitude()), positionY(start_pos.latitude()));
    ezgl::point2d end_xy = ezgl::point2d(positionX(end_pos.longitude()), positionY(end_pos.latitude()));

    //draw the street segment
    r->set_color(col);
    r->set_line_width(5);
    r->draw_line(start_xy, end_xy);
}

void visualDelay (int milliseconds) { // Pause for milliseconds
    std::chrono::milliseconds duration (milliseconds);
    std::this_thread::sleep_for (duration);
}
