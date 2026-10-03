#include "global.h"
#include <omp.h>

std::vector<CourierSubPath> Two_Opt( const std::vector<DeliveryInf>& deliveries, std::vector<CourierSubPath> main_path);


std::vector<CourierSubPath> simulatedAnnealing( const std::vector<DeliveryInf>& deliveries, std::vector<CourierSubPath> main_path);
std::vector<CourierSubPath> generateNeighbor(const std::vector<CourierSubPath>& currentSolution);
bool isValidSolution(const std::vector<CourierSubPath>& solution);




//functions
std::vector<CourierSubPath> greedy(const std::vector<DeliveryInf>& deliveries, IntersectionIdx curr_depot, std::unordered_set<IntersectionIdx>,
                                        std::unordered_set<IntersectionIdx>, std::vector<int>);



double getCourierPathTime(const std::vector<CourierSubPath> courier_route);
CourierSubPath addToPath(IntersectionIdx, IntersectionIdx);

//variables
std::unordered_map<IntersectionIdx, std::multimap<IntersectionIdx, std::pair<double,std::vector<StreetSegmentIdx>>>> travelMatrix_map;








struct DeliveryInfFake {

    // // Constructor
    // DeliveryInfFake(IntersectionIdx pick_up, IntersectionIdx drop_off)
    //     : pickUp(pick_up), dropOff(drop_off) {}

    //The intersection id where the item-to-be-delivered is picked-up.
    IntersectionIdx pickUp;
    bool pickedUp = 0;

    //The intersection id where the item-to-be-delivered is dropped-off.
    IntersectionIdx dropOff;
    bool droppedOff = 0;
};

std::vector<CourierSubPath> greedy(const std::vector<DeliveryInf>& deliveries, IntersectionIdx curr_depot,std::unordered_set<IntersectionIdx> not_picked_up,
std::unordered_set<IntersectionIdx> not_dropped_off,  std::vector<int> num_dropoffs){

    //data structures used
    std::unordered_set<IntersectionIdx> picked_up;
    std::unordered_set<IntersectionIdx> dropped_off;
    std::vector<CourierSubPath> greedyPath;

    //find the closest pick up intersection to the depot
    IntersectionIdx closest_pickup=-1;
    IntersectionIdx currentLocation=-1;

    //find the closest intersection from not picked up set based on travel time
    double min_time = INF;
    for (auto it = not_picked_up.begin(); it != not_picked_up.end(); it++){
       
       //get the travel time from the travel matrix
        double tTime = travelMatrix_map[curr_depot].find(*it)->second.first;
        
        if (tTime < min_time){
            min_time = tTime;
            closest_pickup = *it;
        }
    }
    //set the closest pick up intersection as picked up and remove from not picked up
    picked_up.insert(closest_pickup);
    not_picked_up.erase(closest_pickup); 

    //add to result
    CourierSubPath path = addToPath(curr_depot, closest_pickup);
    greedyPath.push_back(path); 
    currentLocation = closest_pickup; //update the node im currently at

    
    //do this loop until all the intersections have been dropped off
    while (!not_dropped_off.empty()){
        // std::cout << "Not Dropped Off Size: " << not_dropped_off.size() << std::endl;
        // std::cout << "Not Picked Off Size: " << not_picked_up.size() << std::endl;
        //find the closest to current location from not picked up based on time
        double min_time_pick = INF;
        double min_time_drop = INF;

        //if more places to pick up
        
        if(!not_picked_up.empty()){
            for (auto it = not_picked_up.begin(); it != not_picked_up.end(); it++){

                //get the closest pickup location from current node
                double tTime = travelMatrix_map[currentLocation].find(*it)->second.first;
                if (tTime < min_time_pick){
                    min_time_pick = tTime;
                    closest_pickup = *it;
                }
            }
        }   
        //find the closest drop off location from current location
        IntersectionIdx closest_dropoff=-1;
        for (auto it = not_dropped_off.begin(); it != not_dropped_off.end(); it++){
            //get the closest drop off location from current node
            double tTime = travelMatrix_map[currentLocation].find(*it)->second.first;
            if (tTime < min_time_drop){
                min_time_drop = tTime;
                closest_dropoff = *it;
            }
        }

        //print the min times
        std::cout << "Min Time Pick: " << min_time_pick << std::endl;
        std::cout << "Min Time Drop: " << min_time_drop << std::endl;

        //if drop of is closer or nothing to pick up anymore 
        bool valid_drop = false;
        if(not_picked_up.empty() || min_time_drop < min_time_pick){
            //loop through deliveries to see if the drop off is valid
            for (int i = 0; i < deliveries.size(); i++){
                //if the drop off is valid.

                //if its picked up and closest drop off is the current drop off
                if ((picked_up.find(deliveries[i].pickUp) != picked_up.end()) && deliveries[i].dropOff == closest_dropoff){
                
                
                    dropped_off.insert(closest_dropoff);

                    if(num_dropoffs[closest_dropoff] == 1){
                        not_dropped_off.erase(closest_dropoff); 
                    }
                    else{
                        num_dropoffs[closest_dropoff]--;
                    }
                    



                    //not_dropped_off.erase(closest_dropoff); 
                    path = addToPath(currentLocation, closest_dropoff);
                    greedyPath.push_back(path); 
                    currentLocation = closest_dropoff; //update the node im currently at
                    break;
                    
                }
            }
        }
        if (min_time_pick < min_time_drop || !valid_drop){

            //add the pick up to the path
            picked_up.insert(closest_pickup);
            not_picked_up.erase(closest_pickup); 
            path = addToPath(currentLocation, closest_pickup);
            greedyPath.push_back(path); 
            currentLocation = closest_pickup; //update the node im currently at
        }
    }
    //get the path from current location to depot
    path = addToPath(currentLocation, curr_depot);
    greedyPath.push_back(path);

    return greedyPath;
}

//helper function to create a courier sub path
CourierSubPath addToPath(IntersectionIdx start, IntersectionIdx end){
    CourierSubPath path;
    path.intersections = std::make_pair(start,end);
    path.subpath = travelMatrix_map[start].find(end)->second.second;
    return path;
}

// annealing skeleton code
std::vector<CourierSubPath> simulatedAnnealing(const std::vector<DeliveryInf>& deliveries, std::vector<CourierSubPath> main_path) {
    // Initialization
    //std::vector<CourierSubPath> firstSolution = greedy(turn_penalty, deliveries, depots);
    //for (int i = 0; i < firstSolution.size(); i++){
    //std::cout << firstSolution[i].intersections.first << " " << firstSolution[i].intersections.second << std::endl;
    //}
    //std::cout << "GAPPPPPPPPPPPP" << std::endl;
    std::vector<CourierSubPath> currentSolution = Two_Opt(deliveries, main_path);
    for (int i = 0; i < currentSolution.size(); i++){
    std::cout << currentSolution[i].intersections.first << " " << currentSolution[i].intersections.second << std::endl;
    }
    //double currentCost = getCourierPathTime(currentSolution);
   // double initialTemperature = 1000.0;
    //double temperature = initialTemperature;
    //double coolingRate = 0.003;
    // Main Annealing Loop
    /*
    while (temperature > 1.0) {
        // Generate a neighboring solution
        std::vector<CourierSubPath> newSolution = Two_Opt(turn_penalty, deliveries, depots, currentSolution);
        double newCost = getCourierPathTime(newSolution);
        // Calculate cost difference
        double costDifference = newCost - currentCost;
        // Acceptance probability
        double acceptanceProbability = exp(-costDifference / temperature);
        // Accept worse solution with a certain probability
        std::cout << "WEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE" << std::endl;
        for (int i = 0; i < newSolution.size(); i++){
            std::cout << newSolution[i].intersections.first << " " << newSolution[i].intersections.second << std::endl;
        }
        //std::cout << "Cost: " << newCost << " Diff: " << costDifference << " Probability: " << acceptanceProbability << std::endl;
        if (costDifference < 0 || rand() / (RAND_MAX + 1.0) < acceptanceProbability) {
            currentSolution = newSolution;
            currentCost = newCost;
        }
        
        // Cool the temperature
        temperature *= 1 - coolingRate;
    }
    */
    return currentSolution;
}



std::vector<CourierSubPath> travelingCourier(const float turn_penalty, const std::vector<DeliveryInf>& deliveries,const std::vector<IntersectionIdx>& depots){

  //create a vector of all interesting intersections -> pick up, drop off, and depots
    //std::vector<IntersectionIdx> interesting_intersections;
    std::unordered_set<IntersectionIdx> interesting_intersections;
    std::vector<IntersectionIdx> pickup_intersections; 
    std::vector<IntersectionIdx> dropoff_intersections;
    std::vector<IntersectionIdx> srcs;

    //create unordered set that holds all intersections that have not been picked up
    std::unordered_set<IntersectionIdx> not_picked_up;
    std::unordered_set<IntersectionIdx> not_dropped_off;

    //create vector number of pickups and dropoffs at each intersection sized to numIntersections   
    std::vector<int> num_pickups(getNumIntersections(), 0);
    std::vector<int> num_dropoffs(getNumIntersections(), 0);


    travelMatrix_map.clear();

    //get the pick up and drop off intersections
    for (int i = 0; i < deliveries.size(); i++){

        interesting_intersections.insert(deliveries[i].pickUp);
        interesting_intersections.insert(deliveries[i].dropOff);
        srcs.push_back(deliveries[i].pickUp);
        srcs.push_back(deliveries[i].dropOff);

        pickup_intersections.push_back(deliveries[i].pickUp);
        dropoff_intersections.push_back(deliveries[i].dropOff);
        not_picked_up.insert(deliveries[i].pickUp);
        not_dropped_off.insert(deliveries[i].dropOff);

        //increment the number of pickups and dropoffs at each intersection
        num_pickups[deliveries[i].pickUp]++;
        num_dropoffs[deliveries[i].dropOff]++;
    }
    //get the depot intersections
    for (int i = 0; i < depots.size(); i++){
        interesting_intersections.insert(depots[i]);
        srcs.push_back(depots[i]);
    }
    std::cout << "HEllo World" << std::endl;
    //load the travel matrix
    load_travelTime_Matrix(srcs, interesting_intersections,turn_penalty);

    int randomDepot = rand() % depots.size();

    //pick random depot to start from 
    std::vector<CourierSubPath> result= greedy(deliveries, depots[randomDepot], not_picked_up,not_dropped_off, num_dropoffs);


/*

    std::vector<CourierSubPath> temp_greedyPath;
    std::vector<CourierSubPath> greedyPath;
    std::vector<CourierSubPath> annealingPath;

        annealingPath = (simulatedAnnealing(turn_penalty, deliveries, depots, result));
        //temp_greedyPath = (greedy(turn_penalty, deliveries, depots)); 
        //std::cout << "1" << std::endl;
        //greedyPath = Two_Opt(turn_penalty, deliveries, depots, temp_greedyPath);


    int solutionSize = greedyPath.size();
    std::cout << "Solution size initial: " << temp_greedyPath.size() << std::endl;
    std::cout << "Solution size other: " << solutionSize << std::endl;
*/
    return result;
}

double getCourierPathTime(const std::vector<CourierSubPath> courier_route){
    double time = 0;

    double travelTimePath;

    for (int i = 0; i < courier_route.size(); i++){
        travelTimePath = travelMatrix_map.find(courier_route[i].intersections.first)->second.find(courier_route[i].intersections.second)->second.first;
        time += travelTimePath;
    }
    return time;
}


//load the Travel Matrix 
void load_travelTime_Matrix(const std::vector<IntersectionIdx>& srcs, const std::unordered_set<IntersectionIdx>& interesting_intersections, float turn_penalty){

  //loops through each depot and finds the shortest path to all destinations
  //get the current time using chrono
  auto start = std::chrono::high_resolution_clock::now();


  //#pragma omp parallel for //parallelize the for loop
  for (auto src: srcs){
    dijkstraExpansion(turn_penalty, src, interesting_intersections); //returns the shortest path to all destinations from same source
  }

  //get the current time using chrono
  auto finish = std::chrono::high_resolution_clock::now();
  //print the time elapsed
  std::chrono::duration<double> elapsed = finish - start;
  std::cout << "Time taken to load the travel matrix: " << elapsed.count() << " s\n";
}


//function used in m4, dijisktra keeps going until all intersections are found that are being looked for
void dijkstraExpansion(float turn_penalty, IntersectionIdx src, const std::unordered_set<IntersectionIdx>& destMap){
   // bool pathFound=false;
    int numDest=destMap.size();
    int numFound=0;

    //create a hash map intersections sized to the number of intersections
    std::unordered_map<IntersectionIdx, intersection_data > intersectionsHash;

    //create a hash map with visited destinations
    std::unordered_map<IntersectionIdx, bool> visitedDestinations;
    
    //create a variable to store the current intersection as intersect_data type 
    intersection_data srcData=intersections[src];
    srcData.bestTime=INF;
    srcData.reachingEdge=NO_EDGE;
    srcData.visited=true;
    intersectionsHash.insert({src, srcData});
    
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
        intersection_data currData = intersectionsHash[currID];
   
        //checks if the travel time to current node is less than the best time to the same node
        if(currTime < currData.bestTime){
            currData.reachingEdge=wave.edgeId;
            currData.bestTime=wave.travel_time;

            //add the current intersection to the hash 
            intersectionsHash[currID]=currData;

            if (destMap.find(currID) != destMap.end() && !visitedDestinations[currID]){ 
              
                visitedDestinations.insert({currID, true});
                             
                std::vector<StreetSegmentIdx> currPath = bfsTraceBack(currID, intersectionsHash); // Trace back the path
               
                //create the path time and route
                double pathTime = computePathTravelTime(turn_penalty, currPath);
                std::pair<double, std::vector<StreetSegmentIdx>> pathPair(pathTime,currPath);
                
                travelMatrix_map[src].insert({currID, pathPair});
                numFound++;
            }

            // Check if all destinations are found
            if (numFound == numDest) {
                //pathFound = true;
                break;
            }
        
            for(auto &outEdge: findStreetSegmentsOfIntersection(currID)){
                //can drive only on valid roads
                OSMID osm_id = getStreetSegmentInfo(outEdge).wayOSMID;
                std::string road_type = osmRoadMap[osm_id];
                //skip these road types
                if (road_type == "pedestrian" ) {
                    continue;
                }

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
             }
        }
    }
}

//returns the shortest path between the 2 intersections --used in dijkstraExpansion
std::vector<StreetSegmentIdx> bfsTraceBack(IntersectionIdx dest, const std::unordered_map<IntersectionIdx, intersection_data>& intersectionsHash){
    std::vector<StreetSegmentIdx> path;
    //define the destination intersection
    IntersectionIdx curr = dest;
    StreetSegmentIdx prevEdge= intersectionsHash.at(curr).reachingEdge; //get the street segment used to reach the destination intersection

    while(prevEdge != NO_EDGE){

        path.push_back(prevEdge); //add the street segment to the path

        if(segments[prevEdge].one_way){
            curr= getStreetSegmentInfo(prevEdge).from; //get the intersection at the start of the street segment
        }else{
        curr=(curr==getStreetSegmentInfo(prevEdge).from)? getStreetSegmentInfo(prevEdge).to : getStreetSegmentInfo(prevEdge).from;
        }

        prevEdge = intersectionsHash.at(curr).reachingEdge; //get the street segment used to reach the next intersection
    }
    //reverse the path
    std::reverse(path.begin(), path.end());

    return path; //return the path as vector
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TWO_OPT

// You can all this function multiple times in the main loop until the time runs out
std::vector<CourierSubPath> Two_Opt( const std::vector<DeliveryInf>& deliveries, std::vector<CourierSubPath> main_path){
    int main_path_size = 2*(deliveries.size()) + 2;

    std::vector<CourierSubPath> final_path; // vector to be returned
    final_path.resize(main_path_size); // make the vector the correct size

    // At this point, we have a complete CourierSubPath vector with its complete size (main_path)

    // VECTOR FOR INTERSECTIONS
    std::vector<IntersectionIdx> main_path_intersections;
    int main_path_inter_size = main_path_size+1;
    main_path_intersections.resize(main_path_inter_size);

     std::cout << "1" << std::endl;
    // MAP FOR LEGALITY
    std::unordered_map<IntersectionIdx, std::vector<IntersectionIdx>> pickup_vect_map; // map whose value is a vector of all pickups prior to a dropoff 
                                                                                        // the dropoff intersection is the key

    std::unordered_map<IntersectionIdx, std::vector<IntersectionIdx>> instanti_test;
    std::vector<IntersectionIdx> instanti_test_vect;

    // MAP THAT HAS ALL INTERSECTIONS VISITED AND THEIR ORDER
    std::unordered_map<IntersectionIdx, int> main_path_map;

    // Populates main_path_intersections and main_path_map
    for(int i=0; i<main_path_inter_size; i++){
        if(i == main_path_inter_size-1){
            main_path_intersections[i] = main_path[i-1].intersections.second;
            main_path_map.insert({main_path_intersections[i], i});
        }
        else{
            main_path_intersections[i] = main_path[i].intersections.first;
            main_path_map.insert({main_path_intersections[i], i});
        }
    }
     std::cout << "1" << std::endl;
    // Populates pickup_vect_map
    for(IntersectionIdx i=0; i<deliveries.size(); i++){
        std::vector<IntersectionIdx> pickup_vect;

        auto dropoff_iterator = pickup_vect_map.find(deliveries[i].dropOff);
        if(dropoff_iterator == pickup_vect_map.end()){     // iterator could not find the key
            pickup_vect.push_back(deliveries[i].pickUp);
            pickup_vect_map[deliveries[i].dropOff] = pickup_vect;
        }
        else{   // if key does exist and iterator now points to it
            dropoff_iterator->second.push_back(deliveries[i].pickUp);
        }
    }
     std::cout << "1" << std::endl;
    // At this point, we have a vector of the initial vector of intersections (main_path_intersections)
    // We have also built the main map using the vector and a legality-checking map
    std::pair<IntersectionIdx, IntersectionIdx> subpath_longest;
    std::pair<IntersectionIdx, IntersectionIdx> subpath_second_longest;

    int subpath_longest_travel_time = 0;
    int subpath_second_longest_travel_time = 0;


    for(int subpath_idx = 1; subpath_idx < (main_path_size-1); subpath_idx++){ // starting at and subtracting by 1 avoids swapping depots
        double temp_travel_time = travelMatrix_map.find(main_path[subpath_idx].intersections.first)->second.find(main_path[subpath_idx].intersections.second)->second.first;
        
        if(temp_travel_time > subpath_longest_travel_time){
            subpath_second_longest_travel_time = subpath_longest_travel_time;
            subpath_longest_travel_time = temp_travel_time;

            subpath_second_longest.first = subpath_longest.first;
            subpath_second_longest.second = subpath_longest.second;

            subpath_longest.first = main_path[subpath_idx].intersections.first;
            subpath_longest.second = main_path[subpath_idx].intersections.second;
        }
        else if(temp_travel_time > subpath_second_longest_travel_time){
            subpath_second_longest_travel_time = temp_travel_time;

            subpath_second_longest.first = main_path[subpath_idx].intersections.first;
            subpath_second_longest.second = main_path[subpath_idx].intersections.second;
        }
    }
     std::cout << "1" << std::endl;
    // At this point, we have the longest and second longest subpaths
    // Now we create a mini vector that will be swapped/reversed
    // CREATING VECTOR OF ALL INTERSECTIONS TO BE REVERSED
    std::vector<IntersectionIdx> swapping_intersections;
    int num_swapping_intersections;     // number of intersections to be swapped

    int subpath1_end = subpath_longest.second;
    int subpath2_start = subpath_second_longest.first;

    auto iterator_subpath1_end = main_path_map.find(subpath1_end); // gives a pointer to the key in the map
    auto iterator_subpath2_start = main_path_map.find(subpath2_start);

    int dest1 = iterator_subpath1_end->second;  // gives the value of the key (subpath1_end)
    int dest2 = iterator_subpath2_start->second;

    num_swapping_intersections = abs(dest1-dest2);

    swapping_intersections.resize(num_swapping_intersections);

    for(int i=0; i<num_swapping_intersections; i++){
        swapping_intersections[i] = main_path_intersections[dest2-i];
    }
     std::cout << "1" << std::endl;
    // Now we need to check if the swapped path is faster
    //computePathTravelTime(const double turn_penalty, const std::vector<StreetSegmentIdx>& path)
    int total_time_cur = 0;
    int total_time_alt = 0;
    std::cout << dest1 << " " << dest2 << " " << num_swapping_intersections << " " << main_path.size() << std::endl;
    for(int i=0; i<(num_swapping_intersections); i++){
        if(dest1-dest2 < 0){

            //pairTravelPath = travelMatrix_map.find(dropoff_test.first)->second.find(dropoff_test.second)->second.second;
            //travelTimePath = travelMatrix_map.find(main_path[dest1+i-1].intersections.first)->second.find(main_path[dest1+i-1].intersections.second)->second.first;
            IntersectionIdx src = main_path[dest1+i-1].intersections.first;
            IntersectionIdx dest = main_path[dest1+i-1].intersections.second;

            total_time_cur += travelMatrix_map.find(src)->second.find(dest)->second.first;
            std::cout << i << std::endl;
            std::pair<IntersectionIdx, IntersectionIdx> alt_intersection_pair (main_path_intersections[dest1+i-1], main_path_intersections[dest1+i]);
            std::cout << "2" << std::endl;
            total_time_alt += travelMatrix_map.find(alt_intersection_pair.first)->second.find(alt_intersection_pair.second)->second.first;
            std::cout << "3" << std::endl;
        } else {
            
            IntersectionIdx src = main_path[dest2+i-1].intersections.first;
            IntersectionIdx dest = main_path[dest2+i-1].intersections.second;

            total_time_cur += travelMatrix_map.find(src)->second.find(dest)->second.first;
            std::cout << i << std::endl;
            std::pair<IntersectionIdx, IntersectionIdx> alt_intersection_pair (main_path_intersections[dest2+i-1], main_path_intersections[dest2+i]);
            std::cout << "2" << std::endl;
            total_time_alt += travelMatrix_map.find(alt_intersection_pair.first)->second.find(alt_intersection_pair.second)->second.first;
            std::cout << "3" << std::endl;
        }
    }
         std::cout << "1" << std::endl;

    // We now have the current total time and the one for the alternative route to make a comparison
    // We see if alternative path is faster
    if(total_time_cur > total_time_alt){    // if current path is slower than alternative path
     std::cout << "1" << std::endl;

        // Update Map
        for(int i=0; i<num_swapping_intersections; i++){
            auto main_path_iterator = main_path_map.find(swapping_intersections[i]);
            main_path_iterator->second = dest1+i;
        }
     std::cout << "1" << std::endl;

        // We see if the path is legal
        bool is_legal = true;
        for(int i=0; i<num_swapping_intersections; i++){
            IntersectionIdx checking_intersection = swapping_intersections[i];
            std::cout << "Swapping Intersections (checking_intersection): " << checking_intersection << std::endl; // iterator
            std::cout << "A with index i = " << i << std::endl;
            auto checking_inter_iterator = pickup_vect_map.find(checking_intersection);
            
            // Insert debugging code here

            // Print out the contents of the pickup_vect_map
            std::cout << "Size of pickup_vect_map:" << pickup_vect_map.size() << std::endl;
            std::cout << "Contents of pickup_vect_map:" << std::endl;
            for (const auto& entry : pickup_vect_map) {
                std::cout << "Key: " << entry.first << ", Value: ";
                for (const auto& value : entry.second) {
                    std::cout << value << " ";
                }
                std::cout << std::endl;
            }

            // Check the type and value of checking_intersection
            std::cout << "Type of checking_intersection: " << typeid(checking_intersection).name() << std::endl;
            std::cout << "Value of checking_intersection: " << checking_intersection << std::endl;

            std::cout << "Checking intersection: " << checking_intersection << std::endl;
            if (checking_inter_iterator != pickup_vect_map.end()) {
                // Rest of the code to process the iterator
            } else {
                std::cout << "Key " << checking_intersection << " not found in pickup_vect_map!" << std::endl;
            }

            // std::cout << "CHECKING ITERATOR.FIRST " << (checking_inter_iterator->first) << std::endl;
            // std::cout << "CHECKING PICKUP_MAP.FIRST" << (pickup_vect_map.end()->first) << std::endl;

            // std::cout << "CHECKING ITERATOR.SECOND " << (checking_inter_iterator->second)[i] << std::endl;
            //std::unordered_map<IntersectionIdx, std::vector<IntersectionIdx>> tester_map_pick = (pickup_vect_map.end());
            // std::cout << "CHECKING PICKUP_MAP.SECOND" << ((pickup_vect_map.end())->second)[i] << std::endl;

            if (checking_inter_iterator != pickup_vect_map.end()) {
                const auto& pickup_vector = checking_inter_iterator->second;
                std::cout << "Vector size: " << pickup_vector.size() << std::endl;
                if (!pickup_vector.empty()) {
                    std::cout << pickup_vector[0] << std::endl;
                } else {
                    std::cout << "Vector is empty!" << std::endl;
                }
            } else {
                std::cout << "Iterator not found in pickup_vect_map!" << std::endl;
            }

            int checking_int_loc = (main_path_map.find(checking_intersection))->second;
            // std::cout << "Does it fail here? CHECKING_INTER_ITERATOR->SECOND " << (checking_inter_iterator->second)[i] << std::endl; // iterator
            std::cout << "B" << std::endl;
            // std::cout << "CHECKING_INTER_ITERATOR->SECOND" << (checking_inter_iterator->second).size() << std::endl; // iterator
            if (checking_inter_iterator != pickup_vect_map.end()) {
                for(int j=0; j<(checking_inter_iterator->second).size(); j++){ 
                    std::cout << "C" << std::endl;
                    IntersectionIdx required_pickup = checking_inter_iterator->second[j];   // intersection assiciated with a pickup that needs to occur
                    int required_pickup_loc = (main_path_map.find(required_pickup))->second;
                    if(required_pickup_loc > checking_int_loc){
                        is_legal = false;
                        break;
                    }
                }
            }
            
        }
             std::cout << "1" << std::endl;

        // Update map back to original if not legal
        if(is_legal == false){
            for(int i=0; i<num_swapping_intersections; i++){
                auto main_path_iterator = main_path_map.find(swapping_intersections[i]);
                main_path_iterator->second = dest2-i;
            }
        }
        else{
            // if it is legal, then update the vector (map has already been updated)
            for(int i=0; i<num_swapping_intersections; i++){
                main_path_intersections[dest1+i] = swapping_intersections[i];
            }
        }
    }
     std::cout << "1" << std::endl;
    // Finally, we must load the final path
    for(int i=0; i<main_path_size; i++){
        final_path[i].intersections.first = main_path_intersections[i];
        final_path[i].intersections.second = main_path_intersections[i+1];
        final_path[i].subpath = travelMatrix_map.find(final_path[i].intersections.first)->second.find(final_path[i].intersections.second)->second.second;
    }

    return final_path;
}

