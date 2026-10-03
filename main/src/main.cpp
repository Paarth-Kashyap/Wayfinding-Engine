#include <iostream>
#include <string>

#include "m1.h"
#include "m2.h"
#include "m3.h"
#include "m4.h"
#include "global.h"

//Program exit codes
constexpr int SUCCESS_EXIT_CODE = 0;        //Everyting went OK
constexpr int ERROR_EXIT_CODE = 1;          //An error occured
constexpr int BAD_ARGUMENTS_EXIT_CODE = 2;  //Invalid command-line usage

//The default map to load if none is specified
std::string default_map_path = "/cad2/ece297s/public/maps/toronto_canada.streets.bin";
std::vector<IntersectionIdx> destinations_test;

// The start routine (main) for the standalone Wayfinding Engine mapper
// program. This main routine is only used by the interactive application;
// the unit tests have their own main routine and call the functions in
// /libstreetmap/src/ directly.
int main(int argc, char** argv) {

    std::string map_path;
    if(argc == 1) {
        map_path = default_map_path;
    } else if (argc == 2) {
        //Get the map from the command line
        map_path = argv[1];
    } else {
        //Invalid arguments
        std::cerr << "Usage: " << argv[0] << " [map_file_path]\n";
        std::cerr << "  If no map_file_path is provided a default map is loaded.\n";
        return BAD_ARGUMENTS_EXIT_CODE;
    }

    //Load the map and related data structures
    bool load_success = loadMap(map_path);
    if(!load_success) {
        std::cerr << "Failed to load map '" << map_path << "'\n";
        return ERROR_EXIT_CODE;
    }
    std::cout << "Successfully loaded map '" << map_path << "'\n";



    //find the intersection with name "Bloor Street West" and "Spadina Avenue"
   //loop through all intersections     
   for (int i = 0; i < getNumIntersections(); i++) {
       //get the name of the intersection
       std::string intersection_name = getIntersectionName(i);
       //check if the intersection name is "Bloor Street West" and "Spadina Avenue"
       if ((intersections[i].name).find("Bloor Street West & Spadina") != std::string::npos) {
            //src=i;
            std::cout << "Intersection: " << intersections[i].id << std::endl;
            destinations_test.push_back(i);
       }

        // load up destinations
        if ((intersections[i].name).find("College Street & Bay") != std::string::npos) {
            //print intersection name
            std::cout << "Intersection name: " << intersections[i].name << std::endl;
            destinations_test.push_back(i);
        }
        if ((intersections[i].name).find("Yonge Street & Eglinton") != std::string::npos) {
            //print intersection name
            std::cout << "Intersection name: " << intersections[i].name << std::endl;
            destinations_test.push_back(i);
        }
        if ((intersections[i].name).find("Front Street West & York") != std::string::npos) {
            //print intersection name
            std::cout << "Intersection name: " << intersections[i].name << std::endl;
            destinations_test.push_back(i);
        }
        if ((intersections[i].name).find("Eglinton Avenue East & Thermos R") != std::string::npos) {
            //print intersection name
            std::cout << "Intersection name: " << intersections[i].name << std::endl;
            destinations_test.push_back(i);
        }
    }

     //load_travelTime_Matrix(destinations_test,0);
     // Print the travel time matrix
// for (int i = 0; i < destinations_test.size(); i++) {
//     for (int j = 0; j < destinations_test.size(); j++) {
//         // Find the entry corresponding to destinations_test[i]
//         auto it = travelMatrix_map.find(destinations_test[i]);
        
//         // Check if the key exists in the map
//         if (it != travelMatrix_map.end()) {
//             // Look up the multimap associated with the key
//             const auto& multiMap = it->second;
            
//             // Find the travel time associated with destinations_test[j] in the multimap
//             auto timeIt = multiMap.find(destinations_test[j]);
            
//             // Check if the destination pair exists in the multimap
//             if (timeIt != multiMap.end()) {
//                 // Access the travel time from the pair
//                 double tT = timeIt->second.first;
                
//                 // Print the travel time
//                 std::cout << "Travel time from " << intersections[destinations_test[i]].name << " to " << intersections[destinations_test[j]].name << " is " << tT << std::endl;
//             } else {
//                 // Destination pair not found in the multimap
//                 std::cout << "No travel time found from " << intersections[destinations_test[i]].name << " to " << intersections[destinations_test[j]].name << std::endl;
//             }
//         } else {
//             // Key not found in the map
//             std::cout << "Key " << destinations_test[i] << " not found in the travel matrix" << std::endl;
//         }
//     }
// }

    //You can now do something with the map data
    drawMap();

    //Clean-up the map data and related data structures
    std::cout << "Closing map\n";
    closeMap(); 

    return SUCCESS_EXIT_CODE; //exit porgram changed something2.0
}
