#include "global.h"
#include "m3.h"
#include <array>
#include "ui_state.h"

/* map_ui.cpp
   UI / callback / directions code moved from m2.cpp (no behavior change). */

//------------------------------------------------
// UI text helpers
//------------------------------------------------

// The StreetsDatabase API returns the literal "<unknown>" for unnamed ways.
// This converts any unknown/empty name into a clean, user-facing label.
std::string prettyName(const std::string& raw){
   if(raw.empty() || raw == "<unknown>")
      return "Unnamed road";
   return raw;
}

// Intersection names are of the form "A & B & C". Any "<unknown>" component is
// replaced with "Unnamed road" so the status bar never shows the raw token.
static std::string prettyIntersectionName(const std::string& name){
   if(name.empty())
      return "Unnamed intersection";
   std::string out;
   size_t start = 0;
   const std::string sep = " & ";
   while(true){
      size_t pos = name.find(sep, start);
      std::string part = (pos == std::string::npos) ? name.substr(start)
                                                     : name.substr(start, pos - start);
      if(part == "<unknown>" || part.empty())
         part = "Unnamed road";
      out += part;
      if(pos == std::string::npos)
         break;
      out += sep;
      start = pos + sep.size();
   }
   return out;
}

// Friendly display name <-> on-disk map file stem. Only maps that actually
// exist under /cad2/ece297s/public/maps are listed. The combo box shows the
// display names; selection is mapped back to the file stem for loading.
struct MapOption { const char* display; const char* file; };
static const std::array<MapOption, 20> kMapOptions = {{
   {"Select a city…",        ""},
   {"Beijing, China",        "beijing_china"},
   {"Beirut, Lebanon",       "beirut_lebanon"},
   {"Berlin, Germany",       "berlin_germany"},
   {"Boston, USA",           "boston_usa"},
   {"Cape Town, South Africa","cape-town_south-africa"},
   {"Golden Horseshoe, Canada","golden-horseshoe_canada"},
   {"Hainan Island, China",  "hainan-island_china"},
   {"Hamilton, Canada",      "hamilton_canada"},
   {"Iceland",               "iceland"},
   {"Interlaken, Switzerland","interlaken_switzerland"},
   {"Istanbul, Turkey",      "istanbul_turkey"},
   {"London, England",       "london_england"},
   {"New Delhi, India",      "new-delhi_india"},
   {"New York, USA",         "new-york_usa"},
   {"Rio de Janeiro, Brazil","rio-de-janeiro_brazil"},
   {"Saint Helena",          "saint-helena"},
   {"Singapore",             "singapore"},
   {"Tokyo, Japan",          "tokyo_japan"},
   {"Toronto, Canada",       "toronto_canada"},
}};

// Look up the on-disk file stem for a chosen display name ("" if none/invalid).
static std::string mapFileForDisplay(const char* display){
   if(!display) return "";
   for(const auto& opt : kMapOptions){
      if(g_strcmp0(display, opt.display) == 0)
         return opt.file;
   }
   return "";
}

// Loads the GTK CSS stylesheet once and applies it to the whole screen.
static void load_app_stylesheet(){
   static bool loaded = false;
   if(loaded) return;
   GtkCssProvider* provider = gtk_css_provider_new();
   GError* err = nullptr;
   gtk_css_provider_load_from_path(provider, "libstreetmap/resources/style.css", &err);
   if(err){
      g_warning("Could not load style.css: %s", err->message);
      g_error_free(err);
   } else {
      gtk_style_context_add_provider_for_screen(
         gdk_screen_get_default(),
         GTK_STYLE_PROVIDER(provider),
         GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
      loaded = true;
   }
   g_object_unref(provider);
}

// Adds/removes the ".dark" style class on the main window so the CSS dark
// theme follows the Dark Mode toggle.
static void apply_dark_class(ezgl::application* application, bool dark){
   GObject* win_obj = application->get_object(application->get_main_window_id().c_str());
   if(!win_obj) return;
   GtkStyleContext* ctx = gtk_widget_get_style_context(GTK_WIDGET(win_obj));
   if(dark)
      gtk_style_context_add_class(ctx, "dark");
   else
      gtk_style_context_remove_class(ctx, "dark");
}

void act_on_mouse_click(ezgl::application* app, GdkEventButton* /*event*/, double x, double y) {
   // Shows the registered mouse click
   std::cout << "Mouse clicked at (" << x << "," << y << ")\n";
   LatLon pos = LatLon(positionLat(y), positionLon(x));

   // Highlights Closest Intersection
   IntersectionIdx inter_id = (IntersectionIdx) findClosestIntersection(pos);
   std::cout << "Closest Intersection: " << intersections[inter_id].name << "\n";
   intersections[inter_id].highlight = !intersections[inter_id].highlight;

   // Draws path for two intersections
   if(intersection_for_path.second != inter_id){
      intersection_for_path.first = intersection_for_path.second;
      intersection_for_path.second = inter_id;
   }
   
   if(intersection_for_path.first != -1 && intersection_for_path.second != -1){
      GObject* buf = app->get_object("buffer");
      GtkTextBuffer* buffer = GTK_TEXT_BUFFER(buf);

      gtk_text_buffer_set_text(buffer, "", -1);

      dir.str("");

      my_path = findPathBetweenIntersections(15, intersection_for_path);
      my_path_start_end=intersection_for_path;
      show_my_path=true;

      // shows directions of path
      showDirections(my_path);
      std::cout << dir.str() << "wii" << std::endl;
      gtk_text_buffer_set_text(buffer, dir.str().c_str(), -1);
   }
   
   // Shows the Closest POI
   std::pair<POI_data, POIIdx> closest_poi_pair = closest_poi_finder(pos);
   std::stringstream ss;
   std::string inter_name = prettyIntersectionName(intersections[inter_id].name);
   if((closest_poi_pair.first).name == "RBC"){        // Banks, like RBC, are not included in this map's POIs, this means the closest POI  
                                                      // is the one that was initialized, which means no POIs were selected 
      ss << "Intersection: " << inter_name << "    Closest POI: " << "No POI Selected";
   } else {
      ss << "Intersection: " << inter_name << "    Closest POI: " << (closest_poi_pair.first).name;
   }
   app->update_message(ss.str());
   app->refresh_drawing();

}

// Finds the closest POI to the click position
std::pair<POI_data, POIIdx> closest_poi_finder(LatLon my_position){
   POI_data closest_poi = pois[8];                    // The closest POI is initialized to be a bank (used to determine if POIs were selected)
   POIIdx closest_poi_idx = -1;

   std::cout << "Closest POI: " << closest_poi.name << std::endl;

   std::pair<POI_data, POIIdx> closest_pair;
   closest_pair.first = closest_poi;
   closest_pair.second = closest_poi_idx;

   closest_pair = closest_poi_finder_helper(leisure_POIs, leisure_pois, my_position, closest_pair);
   closest_pair = closest_poi_finder_helper(food_POIs, food_pois, my_position, closest_pair);
   closest_pair = closest_poi_finder_helper(driving_POIs, driving_pois, my_position, closest_pair);
   closest_pair = closest_poi_finder_helper(school_POIs, school_pois, my_position, closest_pair);
   closest_pair = closest_poi_finder_helper(emergency_POIs, emergency_pois, my_position, closest_pair);

   std::cout << "Closest POI: " << closest_poi.name << std::endl;

   return closest_pair;
}

// Helper function to find closest poi
std::pair<POI_data, POIIdx> closest_poi_finder_helper(bool poi_bool, std::vector<POI_data> poi_vector, LatLon my_position, std::pair<POI_data, POIIdx> closest_pair){
   POI_data closest_poi = closest_pair.first;
   POIIdx closest_poi_idx = closest_pair.second;

   if (poi_bool == true){
      for (size_t poi_id = 0; poi_id < poi_vector.size(); ++poi_id){
         if(findDistanceBetweenTwoPoints(std::make_pair(my_position, poi_vector[poi_id].LatLon_loc)) < findDistanceBetweenTwoPoints(std::make_pair(my_position, closest_poi.LatLon_loc))){
            closest_poi_idx = findClosestPOI(my_position, poi_vector[poi_id].name);
            if(closest_poi_idx != -1){
               closest_poi = poi_vector[poi_id];
            }
         }
      }
   }

   closest_pair.first = closest_poi;
   closest_pair.second = closest_poi_idx;

   return closest_pair;
}

// This gets called to initialized all buttons and entries to then be used in later functions
void initial_setup (ezgl::application* application, bool /*new_window*/){

   // Apply the modern stylesheet before building the rest of the UI.
   load_app_stylesheet();

   // Find button
   GObject* find_object = application->get_object("FindButton");
   if (find_object != nullptr){
      GtkButton* find_button = GTK_BUTTON(find_object);
      g_signal_connect(find_button, "clicked", G_CALLBACK(toggle_find), application);
   }

   // Clear Path button
   GObject* clear_object = application->get_object("ClearPathButton");
   if (clear_object != nullptr){
      GtkButton* clear_button = GTK_BUTTON(clear_object);
      g_signal_connect(clear_button, "clicked", G_CALLBACK(toggle_clear), application);
   }

   // Find street 1
   GObject* list_store_1 = application->get_object("liststore1");
   GtkListStore *list_1 = GTK_LIST_STORE(list_store_1);
   
   GObject* entry_1 = application->get_object("Entry1");
   GtkEntry* text_entry_1 = GTK_ENTRY(entry_1);
   g_signal_connect(text_entry_1, "changed", G_CALLBACK(on_entry_changed_1), list_1);

   GObject* entry_completion_1 = application->get_object("entrycompletion1");
   GtkEntryCompletion *entry_comp_1 = GTK_ENTRY_COMPLETION(entry_completion_1);
   gtk_entry_completion_set_model(entry_comp_1, GTK_TREE_MODEL(list_1));
   gtk_entry_completion_set_text_column(entry_comp_1, 0);

   // Find street 2
   GObject* list_store_2 = application->get_object("liststore2");
   GtkListStore *list_2 = GTK_LIST_STORE(list_store_2);
   
   GObject* entry_2 = application->get_object("Entry2");
   GtkEntry* text_entry_2 = GTK_ENTRY(entry_2);
   g_signal_connect(text_entry_2, "changed", G_CALLBACK(on_entry_changed_2), list_2);

   GObject* entry_completion_2 = application->get_object("entrycompletion2");
   GtkEntryCompletion *entry_comp_2 = GTK_ENTRY_COMPLETION(entry_completion_2);
   gtk_entry_completion_set_model(entry_comp_2, GTK_TREE_MODEL(list_2));
   gtk_entry_completion_set_text_column(entry_comp_2, 0);

   // Find street 2
   GObject* list_store_11 = application->get_object("liststore11");
   GtkListStore *list_11 = GTK_LIST_STORE(list_store_11);
   
   GObject* entry_11 = application->get_object("Entry11");
   GtkEntry* text_entry_11 = GTK_ENTRY(entry_11);
   g_signal_connect(text_entry_11, "changed", G_CALLBACK(on_entry_changed_11), list_11);

   GObject* entry_completion_11 = application->get_object("entrycompletion11");
   GtkEntryCompletion *entry_comp_11 = GTK_ENTRY_COMPLETION(entry_completion_11);
   gtk_entry_completion_set_model(entry_comp_11, GTK_TREE_MODEL(list_11));
   gtk_entry_completion_set_text_column(entry_comp_11, 0);
   
   // Find street 2
   GObject* list_store_22 = application->get_object("liststore22");
   GtkListStore *list_22 = GTK_LIST_STORE(list_store_22);
   
   GObject* entry_22 = application->get_object("Entry22");
   GtkEntry* text_entry_22 = GTK_ENTRY(entry_22);
   g_signal_connect(text_entry_22, "changed", G_CALLBACK(on_entry_changed_22), list_22);

   GObject* entry_completion_22 = application->get_object("entrycompletion22");
   GtkEntryCompletion *entry_comp_22 = GTK_ENTRY_COMPLETION(entry_completion_22);
   gtk_entry_completion_set_model(entry_comp_22, GTK_TREE_MODEL(list_22));
   gtk_entry_completion_set_text_column(entry_comp_22, 0);

   // Search bar
   GObject* list_store_3 = application->get_object("liststore3");
   GtkListStore *list_3 = GTK_LIST_STORE(list_store_3);
   
   GObject* entry_3 = application->get_object("SearchBar");
   GtkEntry* text_entry_3 = GTK_ENTRY(entry_3);
   g_signal_connect(text_entry_3, "changed", G_CALLBACK(on_entry_changed_3), list_3);
   g_signal_connect(text_entry_3, "activate", G_CALLBACK(on_entry_activated_3), application);

   GObject* entry_completion_3 = application->get_object("entrycompletion3");
   GtkEntryCompletion *entry_comp_3 = GTK_ENTRY_COMPLETION(entry_completion_3);
   gtk_entry_completion_set_model(entry_comp_3, GTK_TREE_MODEL(list_3));
   gtk_entry_completion_set_text_column(entry_comp_3, 0);

   // Create new buttons
   application->create_button ("Dark Mode", 6, toggle_dark_mode);
   application->create_button ("Show Leisure POIs", 0, 12, 1, 2, toggle_leisure);
   application->create_button ("Show Food POIs", 1, 12, 1, 2, toggle_food);
   application->create_button ("Show Driving POIs", 2, 12, 1, 2, toggle_driving);
   application->create_button ("Show School POIs", 0, 14, 1, 2, toggle_school);
   application->create_button ("Show Emergency POIs", 1, 14, 1, 2, toggle_emergency);
   application->create_button ("Show Subway Stations", 2, 14, 1, 2, toggle_subway_stations);
   application->create_button ("Help", 0, 23, 3, 12, toggle_help);
   // Drop down menu
   std::vector<std::string> map_choices;
   map_choices.reserve(kMapOptions.size());
   for(const auto& opt : kMapOptions)
      map_choices.emplace_back(opt.display);
   application->create_combo_box_text(
      "Change Map",
      8,
      combo_box_cbk,
      map_choices
   );
}

// Processing entry for street 1
void on_entry_changed_1(GtkEntry *entry, gpointer user_data){
   const gchar *text = gtk_entry_get_text(entry);

   GtkListStore *list_1 = GTK_LIST_STORE(user_data);
   
   gtk_list_store_clear(list_1);
   
   std::vector<StreetIdx> suggestions = findStreetIdsFromPartialStreetName(text);

   std::set<std::string> added;

   if (suggestions.size() < 100){
      for (int i = 0; i < suggestions.size(); i++){
         std::string streetName = getStreetName(suggestions[i]);
         if(added.find(streetName) == added.end()){
            GtkTreeIter iter;
            gtk_list_store_append(list_1, &iter);
            gtk_list_store_set(list_1, &iter, 0, streetName.c_str(), -1);
            added.insert(streetName);
         }
      }
   }
}

// Processing entry for street 2
void on_entry_changed_2(GtkEntry *entry, gpointer user_data){
   const gchar *text = gtk_entry_get_text(entry);

   GtkListStore *list_2 = GTK_LIST_STORE(user_data);
   
   gtk_list_store_clear(list_2);
   
   std::vector<StreetIdx> suggestions = findStreetIdsFromPartialStreetName(text);

   std::set<std::string> added;

   if (suggestions.size() < 100){
      for (int i = 0; i < suggestions.size(); i++){
         std::string streetName = getStreetName(suggestions[i]);
         if(added.find(streetName) == added.end()){
            GtkTreeIter iter;
            gtk_list_store_append(list_2, &iter);
            gtk_list_store_set(list_2, &iter, 0, streetName.c_str(), -1);
            added.insert(streetName);
         }
      }
   }
}

// Processing entry for search bar
void on_entry_changed_3(GtkEntry *entry, gpointer user_data){
   const gchar *text = gtk_entry_get_text(entry);

   GtkListStore *list_3 = GTK_LIST_STORE(user_data);
   
   gtk_list_store_clear(list_3);
   
   std::vector<StreetIdx> suggestions = findStreetIdsFromPartialStreetName(text);

   std::set<std::string> added;

   if (suggestions.size() < 100){
      for (int i = 0; i < suggestions.size(); i++){
         std::string streetName = getStreetName(suggestions[i]);
         if(added.find(streetName) == added.end()){
            GtkTreeIter iter;
            gtk_list_store_append(list_3, &iter);
            gtk_list_store_set(list_3, &iter, 0, streetName.c_str(), -1);
            added.insert(streetName);
         }
      }
   }
}

// Processing enter button for search bar
void on_entry_activated_3(GtkEntry *entry, ezgl::application* application){
   const gchar *text = gtk_entry_get_text(entry);

   LatLon new_latlon(positionLat(center_world.y), positionLon(center_world.x));

   std::vector<StreetIdx> suggestions = findStreetIdsFromPartialStreetName(text);
   if (!suggestions.empty()){
      std::cout << suggestions.size() << std::endl;
      
      std::vector<IntersectionIdx> intersections_temp = findIntersectionsOfStreet(suggestions[0]);
      if (!intersections_temp.empty()){
         IntersectionIdx closest = 0;
         double currentdistance = 10000000000;
         for (int i = 0; i < intersections_temp.size(); ++i){
            LatLon interposition = getIntersectionPosition(intersections_temp[i]);

            double distance = findDistanceBetweenTwoPoints(std::make_pair(interposition, new_latlon));

            std::cout << distance << std::endl;

            if(distance < currentdistance){
               currentdistance = distance;
               closest = intersections_temp[i];
            }
         }

         double lat_temp = getIntersectionPosition(closest).latitude();
         double lon_temp = getIntersectionPosition(closest).longitude();
         double new_x_temp = positionX(lon_temp);
         double new_y_temp = positionY(lat_temp);
         std::cout << lat_temp << " " << lon_temp << " " << new_x_temp << " " << new_y_temp << std::endl;
         ezgl::rectangle new_visible({new_x_temp - 200, new_y_temp - 200}, {new_x_temp + 200, new_y_temp + 200});


         std::cout << "DISPAYS CENTER:" << new_visible.center_x() << " " << new_visible.center_y() << std::endl;
         application->change_canvas_world_coordinates("MainCanvas", new_visible);

         application->refresh_drawing();
      }
   }
}

// Processing entry for end street 1
void on_entry_changed_11(GtkEntry *entry, gpointer user_data){
   const gchar *text = gtk_entry_get_text(entry);

   GtkListStore *list_11 = GTK_LIST_STORE(user_data);
   
   gtk_list_store_clear(list_11);
   
   std::vector<StreetIdx> suggestions = findStreetIdsFromPartialStreetName(text);

   std::set<std::string> added;

   if (suggestions.size() < 100){
      for (int i = 0; i < suggestions.size(); i++){
         std::string streetName = getStreetName(suggestions[i]);
         if(added.find(streetName) == added.end()){
            GtkTreeIter iter;
            gtk_list_store_append(list_11, &iter);
            gtk_list_store_set(list_11, &iter, 0, streetName.c_str(), -1);
            added.insert(streetName);
         }
      }
   }
}

   // Processing entry for end street 2
void on_entry_changed_22(GtkEntry *entry, gpointer user_data){
   const gchar *text = gtk_entry_get_text(entry);

   GtkListStore *list_22 = GTK_LIST_STORE(user_data);
   
   gtk_list_store_clear(list_22);
   
   std::vector<StreetIdx> suggestions = findStreetIdsFromPartialStreetName(text);

   std::set<std::string> added;

   if (suggestions.size() < 100){
      for (int i = 0; i < suggestions.size(); i++){
         std::string streetName = getStreetName(suggestions[i]);
         if(added.find(streetName) == added.end()){
            GtkTreeIter iter;
            gtk_list_store_append(list_22, &iter);
            gtk_list_store_set(list_22, &iter, 0, streetName.c_str(), -1);
            added.insert(streetName);
         }
      }
   }
}


// Processing entry for drop down
void combo_box_cbk(GtkComboBoxText* self, ezgl::application* app){
   gchar* map_select_text = gtk_combo_box_text_get_active_text(self);
   if (!map_select_text)
      return;

   // Map the friendly display name back to the on-disk file stem.
   std::string file = mapFileForDisplay(map_select_text);
   g_free(map_select_text);

   if (file.empty())   // "Select a city…" or unrecognized -> do nothing
      return;

   // Pick a font that can render the local script.
   china_font = (file == "beijing_china" || file == "hainan-island_china");
   japan_font = (file == "tokyo_japan");
   iran_font  = false; // no Persian-script map currently available

   closeMap();
   std::string new_map_path = "/cad2/ece297s/public/maps/" + file + ".streets.bin";
   loadMap(new_map_path);

   ezgl::rectangle different_world({min_x, min_y}, {max_x, max_y});
   app->change_canvas_world_coordinates("MainCanvas", different_world);
   app->refresh_drawing();
}

void toggle_dark_mode (GtkWidget* /*widget*/, ezgl::application* application){
   dark_mode = !dark_mode;
   apply_dark_class(application, dark_mode); // restyle the control panel too
   application->refresh_drawing();
}

void toggle_leisure (GtkWidget* /*widget*/, ezgl::application* application){
   leisure_POIs = !leisure_POIs;
   application->refresh_drawing();
}

void toggle_food (GtkWidget* /*widget*/, ezgl::application* application){
   food_POIs = !food_POIs;
   application->refresh_drawing();
}

void toggle_driving (GtkWidget* /*widget*/, ezgl::application* application){
   driving_POIs = !driving_POIs;
   application->refresh_drawing();
}

void toggle_school (GtkWidget* /*widget*/, ezgl::application* application){
   school_POIs = !school_POIs;
   application->refresh_drawing();
}

void toggle_emergency (GtkWidget* /*widget*/, ezgl::application* application){
   emergency_POIs = !emergency_POIs;
   application->refresh_drawing();
}

void toggle_subway_stations (GtkWidget* /*widget*/, ezgl::application* application){
   subway_station_POIs = !subway_station_POIs;
   application->refresh_drawing();
}


void toggle_help (GtkWidget* /*widget*/, ezgl::application* application){
   help_popup = !help_popup;
   if(help_popup == true){
      application->create_popup_message("How to Use the Map", "DRAGGING, ZOOMING, AND CLICKING\n"
                                       "Use the arrow buttons, or drag using the mouse to move along the map.\n"
                                       "Zoom in and out using the buttons or by mouse scrolling. Or click zoom fit to see the entire city.\n"
                                       "Click anywhere on the map to find the closest intersection and point of interest.\n\n"
                                       "BUTTONS\n"
                                       "Select the Dark Mode button to turn on dark mode, or deselect to turn it off.\n"
                                       "To see points of interest on the map, select the appropriate POI buttons, or deselect to turn them off.\n\n"
                                       "CHANGING LOCATIONS\n"
                                       "Change locations by clicking the button and selecting the desired city. \n"
                                       "Go straight to a street by entering a street in the search bar.\n\n"
                                       "FINDING A ROUTE\n"
                                       "Click in two places to see a highlighted route between them.\n"
                                       "Or find a route on the map by typing in two pairs of intersecting streets and clicking the Find button.\n"
                                       "Directions are displayed on the bottom right corner.\n\n"
                                       "CLEARING A ROUTE\n"
                                       "Clear a route by clicking the Clear Path button.\n\n"
                                       "EXIT\n"
                                       "Exit the program by clicking the Proceed button.\n\n");
   }
   application->refresh_drawing();
}

// Finds the direction of the segment and next segment
std::string findCardinal(LatLon start_curr, LatLon end_curr){
   double delta_lon = end_curr.longitude() - start_curr.longitude();
   double delta_lat = end_curr.latitude() - start_curr.latitude();

   double angl = atan2(delta_lat, delta_lon);
   double angle = angl*180/M_PI;
   
   if (angle > 45 && angle <= 135)
      return "North";
   else if (angle > -45 && angle <= 45)
      return "East";
   else if (angle > -135 && angle <= -45)
      return "South";
   else
      return "West";
}

// Determine whether we should turn left, right or go straight
std::string determineDirection(const std::string& curr_dir, const std::string& next_dir){
    if (curr_dir == next_dir) {
        return "Continue straight";
    } else if ((curr_dir == "North" && next_dir == "East") ||
               (curr_dir == "East" && next_dir == "South") ||
               (curr_dir == "South" && next_dir == "West") ||
               (curr_dir == "West" && next_dir == "North")) {
        return "Turn left";
    } else {
        return "Turn right";
    }
}

// Display the direction
void showDirections(const std::vector<StreetSegmentIdx> path){
      double total_distance = 0.0;
      for (size_t i = 1; i < path.size() - 1; ++i){
         
         

         StreetSegmentInfo previous_segment = getStreetSegmentInfo(path[i-1]);
         StreetSegmentInfo current_segment = getStreetSegmentInfo(path[i]);
         StreetSegmentInfo next_segment = getStreetSegmentInfo(path[i - 1]);

         StreetSegmentInfo next_next_segment = getStreetSegmentInfo(path[i+1]);

         current_direction = findCardinal(getIntersectionPosition(current_segment.from), getIntersectionPosition(current_segment.to));
         next_direction = findCardinal(getIntersectionPosition(next_segment.from), getIntersectionPosition(next_segment.to));

         std::string turn_direction = determineDirection(current_direction, next_direction);
         std::string curr_street_name = getStreetName(current_segment.streetID);
         std::string prev_street_name = getStreetName(previous_segment.streetID);

         if (prev_street_name != curr_street_name){

            total_distance = std::round(total_distance * 0.2)*5;     //rounding distance to multiple of 5
            double minutes = std::round(((total_distance/current_segment.speedLimit)/60)*100)/100;
            double int_minutes = static_cast<int>(minutes);
            double seconds = static_cast<int>((minutes - int_minutes) * 60);
            dir << "Travel along " << prettyName(prev_street_name) << " for " << total_distance << " meters. (Approx: " << int_minutes << " minutes and " << seconds << " seconds). \n";

            total_distance = 0.0;
         }

         if (prev_street_name != curr_street_name){
            // Print the direction instruction
            dir << turn_direction << " at " << prettyName(getStreetName(next_next_segment.streetID)) << "\n";
         }

         
         double segment_length = findDistanceBetweenTwoPoints(std::make_pair(getIntersectionPosition(current_segment.from), getIntersectionPosition(current_segment.to)));

         total_distance += segment_length;

         int iplus = i + 2;

         if (iplus == path.size()){
            total_distance = std::round(total_distance * 0.2)*5;     //rounding distance to multiple of 5
            double minutes = std::round(((total_distance/current_segment.speedLimit)/60)*100)/100;
            double int_minutes = static_cast<int>(minutes);
            double seconds = static_cast<int>((minutes - int_minutes) * 60);
            dir << "Travel along " << prettyName(curr_street_name) << " for " << total_distance << " meters. (Approx: " << int_minutes << " minutes and " << seconds << " seconds). \n";

            total_distance = 0.0;
         }
         
      }
      
      
   }

// Clear button to clear route
void toggle_clear (GtkWidget* /*widget*/, ezgl::application* application){
   my_path.clear();
   show_my_path=false;
   intersection_for_path.first = -1;
   intersection_for_path.second = -1;
   my_path_start_end = intersection_for_path;

   draw_highlighted_route(application->get_renderer(), my_path, my_path_start_end);
   application->flush_drawing();
}


// Find button for two streets intersection
void toggle_find (GtkWidget* /*widget*/, ezgl::application* application){

   GObject* gtk_object1 = application->get_object("Entry1");
   GtkEntry* gtk_entry1 = GTK_ENTRY(gtk_object1);
   const gchar* text1 = gtk_entry_get_text(gtk_entry1); // take in text from each entry,

   GObject* gtk_object2 = application->get_object("Entry2");
   GtkEntry* gtk_entry2 = GTK_ENTRY(gtk_object2);
   const gchar* text2 = gtk_entry_get_text(gtk_entry2); // take in text from each entry,

   GObject* gtk_object11 = application->get_object("Entry11");
   GtkEntry* gtk_entry11 = GTK_ENTRY(gtk_object11);
   const gchar* text11 = gtk_entry_get_text(gtk_entry11); // take in text from each entry,

   GObject* gtk_object22 = application->get_object("Entry22");
   GtkEntry* gtk_entry22 = GTK_ENTRY(gtk_object22);
   const gchar* text22 = gtk_entry_get_text(gtk_entry22); // take in text from each entry,

   std::cout << text1 << " " << text2 << "   " << text11 << " " << text22 << std::endl;

   std::vector<StreetIdx> streetOptions1;
   std::vector<StreetIdx> streetOptions2;
   std::vector<StreetIdx> streetOptions11;
   std::vector<StreetIdx> streetOptions22;
   
   if (text1[0] != '\0' && text2[0] != '\0' && text11[0] != '\0' && text22[0] != '\0'){
      streetOptions1 = findStreetIdsFromPartialStreetName(text1);
      streetOptions2 = findStreetIdsFromPartialStreetName(text2);
      streetOptions11 = findStreetIdsFromPartialStreetName(text11);
      streetOptions22 = findStreetIdsFromPartialStreetName(text22);   
   }

   std::vector<IntersectionIdx> list1;
   std::vector<IntersectionIdx> list2;

   
   if(!streetOptions1.empty() && !streetOptions2.empty()){
      StreetIdx streetId1 = streetOptions1.front();
      StreetIdx streetId2 = streetOptions2.front();
      list1 = findIntersectionsOfTwoStreets(streetId1, streetId2);
   }

   if(!streetOptions11.empty() && !streetOptions22.empty()){
      StreetIdx streetId11 = streetOptions11.front();
      StreetIdx streetId22 = streetOptions22.front();
      list2 = findIntersectionsOfTwoStreets(streetId11, streetId22);
   }
   
   std::stringstream error;

   // Checks to see if there is no entries at all
   if (text1[0] != '\0' && text2[0] != '\0' && text11[0] != '\0' && text22[0] != '\0'){
      if(!list1.empty() && !list2.empty()){

         IntersectionIdx inter1 = list1.front();
         IntersectionIdx inter2 = list2.front();

         std::pair<IntersectionIdx, IntersectionIdx> navigation_pair = std::make_pair(inter1, inter2);

         std::cout << navigation_pair.first << " " << navigation_pair.second << std::endl;

         GObject* buf = application->get_object("buffer");
         GtkTextBuffer* buffer = GTK_TEXT_BUFFER(buf);

         gtk_text_buffer_set_text(buffer, "", -1);
         

         dir.str("");


         my_path = findPathBetweenIntersections(15.00, navigation_pair);
         my_path_start_end = navigation_pair;
         show_my_path=true;

         

         showDirections(my_path);
         
         std::cout << dir.str() << "wii" << std::endl;

         
         gtk_text_buffer_set_text(buffer, dir.str().c_str(), -1);
         
      } else {
         // Below is various error checks which are displayed on a pop-up message
         if (list1.empty() && (!streetOptions1.empty() && !streetOptions2.empty())){
            error << text1 << " and " << text2 << " is an invalid intersection.";
         }
         if (list2.empty() && (!streetOptions11.empty() && !streetOptions22.empty())){
            error << text11 << " and " << text22 << " is an invalid intersection.";
         }
         if (streetOptions1.empty()){
            error << " " << text1 << " is an invalid street name.";
         }
         if (streetOptions2.empty()){
            error << " " << text2 << " is an invalid street name.";
         }
         if (streetOptions11.empty()){
            error << " " << text11 << " is an invalid street name.";
         }
         if (streetOptions22.empty()){
            error << " " << text22 << " is an invalid street name.";
         }
         showErrorDialog(error);

         
      }
   } else {
      error << " Please fill in all streets to find a route.";
      showErrorDialog(error);
   }
   
   
}

// Function to show error dialog on a pop-up
void showErrorDialog(const std::stringstream& message){
   std::string errorMessage = message.str();
   GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s", errorMessage.c_str());

   gtk_window_set_title(GTK_WINDOW(dialog), "Input Error");

   gtk_dialog_run(GTK_DIALOG(dialog));

   gtk_widget_destroy(dialog);
}

