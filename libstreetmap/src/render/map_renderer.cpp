#include "global.h"
#include "m3.h"
#include <array>
#include "ui_state.h"
#include "spatial_grid.h"

/* map_renderer.cpp
   Rendering-domain code moved from m2.cpp (no behavior change).
   DEFINES the shared file-scope render/UI state (declared extern in ui_state.h). */

/* ---- shared file-scope state DEFINITIONS (moved from m2.cpp) ---- */
std::vector<StreetSegmentIdx> my_path;
std::pair<IntersectionIdx, IntersectionIdx> my_path_start_end;

bool dark_mode = false;
bool leisure_POIs = false;
bool food_POIs = false;
bool driving_POIs = false;
bool school_POIs = false;
bool emergency_POIs = false;
bool subway_station_POIs = false;
bool help_popup = false;
bool show_my_path=false; 
bool visualize_search = false; // animate A* exploration when true

double worldWidth;
double worldHeight; 
double screenWidth;
double screenHeight;
double scaleFactor;
double worldArea;
double scale;
double initialScale = 1000;
bool china_font = false;
bool iran_font = false;
bool japan_font = false;
ezgl::point2d center_world;
std::vector<std::string> list_of_directions;

std::pair<IntersectionIdx, IntersectionIdx> intersection_for_path (-1, -1);
std::string current_direction = "";
std::string next_direction = "";
std::stringstream dir;

/* ---- A* search visualization state ---- */
bool               search_vis_enabled = false;
ezgl::application* search_vis_app = nullptr;
ezgl::renderer*    search_vis_renderer = nullptr;
int                search_vis_delay_ms = 1;

void drawMap() {
   // Create the ezgl application
   ezgl::application::settings settings;
   
   settings.main_ui_resource = "libstreetmap/resources/main.ui";
   settings.window_identifier = "MainWindow";
   settings.canvas_identifier = "MainCanvas";
   ezgl::application application(settings);

   ezgl::rectangle initial_world(
                                 {(min_x), (min_y)},
                                 {(max_x), (max_y)});

   application.add_canvas("MainCanvas", draw_main_canvas, initial_world);
   application.run(initial_setup, act_on_mouse_click, nullptr, nullptr);
   
}

// Callback functions
// Prints the frame render time and FPS to the terminal on every refresh.
void draw_main_canvas (ezgl::renderer *g){
   auto frame_start = std::chrono::steady_clock::now();

   // Query camera state once per frame.
   ezgl::rectangle vw = g->get_visible_world();
   ezgl::rectangle vs = g->get_visible_screen();
   worldHeight = vw.height();
   worldWidth  = vw.width();
   screenHeight = vs.height();
   screenWidth  = vs.width();
   worldArea = vw.area();
   scaleFactor = worldWidth/screenWidth;
   center_world = vw.center();

   //set scale length (10% of the screen) and derive the zoom scale
   scale = scaleFactor * (0.1 * screenWidth);
   g->set_coordinate_system(ezgl::WORLD);

   draw_dark_mode(g);
   draw_features(g);
   draw_segments(g);
   draw_highlighted_route(g, my_path, my_path_start_end);
   draw_POI(g);
   drawDetailOSM(g);

   // Report render performance on every refresh.
   auto frame_end = std::chrono::steady_clock::now();
   double frame_sec = std::chrono::duration<double>(frame_end - frame_start).count();
   double fps = (frame_sec > 0.0) ? (1.0 / frame_sec) : 0.0;
   std::cout << "[render] " << (frame_sec * 1000.0) << " ms  |  "
             << fps << " fps" << std::endl;
}

//draws all streets, highways, paths, etc.
void draw_segments(ezgl::renderer *g){
   // Define line widths for different road types at various scales
   struct LineWidths {
      double highway;
      double highway_link;
      double primary;
      double secondary;
      double other;
      double other_res;
   };
   LineWidths widths;
   //dictates the width of the street based on scale
   if (scale > 2500) {
      widths = {5, 2, 3, 0, 0, 0};
   }if (scale < 2500) {
      widths = {5, 2, 3, 2, 0, 0};
   }if (scale < 1000) {
      widths = {6, 3, 3, 2, .5, 0};
   }if (scale < 700) {
      widths = {10, 8, 6, 4, 1, 0};
   }if (scale < 500){
      widths = {10, 8, 6, 4, 1, 1};
   } if (scale < 200) {
      widths = {10, 8, 7, 6, 3, 3};
   }

   ezgl::rectangle visible_world = g->get_visible_world();

   // Visit only segments whose grid cells overlap the view, instead of
   // scanning the whole array. Falls back to a full scan if the grid is unset.
   auto draw_one_segment = [&](int i){
      if(!intersects(visible_world,segments[i].seg_rect)){
         return;
      }

      // Set line width and color from the precomputed road class (no API
      // calls, map lookups, or string compares in the per-frame hot path).
      double line_width = 0;
      ezgl::color color;
      bool draw_segment = false;

      switch (segments[i].road_class) {
         case RoadClass::OTHER:
            color = dark_mode ? DM_SMALL_STREETS : LIGHT_GREY;
            line_width = widths.other_res;
            draw_segment = true;
            break;
         case RoadClass::TERTIARY:
            color = dark_mode ? DM_SMALL_STREETS : LIGHT_GREY;
            line_width = widths.other;
            draw_segment = true;
            break;
         case RoadClass::SECONDARY:
            color = dark_mode ? DM_SMALL_STREETS : LIGHT_GREY;
            line_width = widths.secondary;
            draw_segment = true;
            break;
         case RoadClass::LINK:
            color = dark_mode ? DM_LARGE_STREETS : DARK_GREY;
            line_width = widths.highway_link;
            draw_segment = true;
            break;
         case RoadClass::PRIMARY:
            color = dark_mode ? DM_SMALL_STREETS : LIGHT_GREY;
            line_width = widths.primary;
            draw_segment = true;
            break;
         case RoadClass::HIGHWAY:
            color = dark_mode ? DM_LARGE_STREETS : DARK_GREY;
            line_width = widths.highway;
            draw_segment = true;
            break;
         case RoadClass::NONE:
         default:
            draw_segment = false;
            break;
      }
      if (draw_segment)
         segments[i].width = line_width;

      // Draw the segment if needed
      if (draw_segment && line_width > 0) {
         g->set_line_width(line_width);
         g->set_line_cap(ezgl::line_cap::round); // Round ends
         displaySeg(segments[i], g, color, line_width);
         // Show name of segment
         draw_street_names(g, segments[i]);
      }
   };

   if (g_segment_grid.ready()) {
      g_segment_grid.query(visible_world, (int)segments.size(), draw_one_segment);
   } else {
      for (size_t i = 0; i < segments.size(); ++i) draw_one_segment((int)i);
   }
}
void draw_dark_mode(ezgl::renderer *g){
   if (dark_mode){
      ezgl::rectangle visible_world = g->get_visible_world();
      g->set_color(DM_BACKGROUND);
      g->fill_rectangle(visible_world);
      
   }
}

//draws all intersections
void draw_intersections(ezgl::renderer *g){
   ezgl::rectangle visible_world = g->get_visible_world();
   double radius=4;   
   for (size_t i = 0; i < intersections.size(); ++i){
      
      //bound checking
      if(!(visible_world.contains(intersections[i].xy_loc) && scale<155)){
         continue;
      }

      //change colour to black
      if(dark_mode == true){
         if (intersections[i].highlight){
            g->set_color(DM_HIGHLIGHT_COL);
            radius=8;
      } else {
            radius=4;
            g->set_color(ezgl::WHITE);
         }
      } else{
         if (intersections[i].highlight){
            radius=8;
            g->set_color(ezgl::BLACK);
         } else {
            radius=4;
            g->set_color(DARK_GREY);
         }
      }
      ezgl::point2d inter_loc = intersections[i].xy_loc;
      g->fill_arc(inter_loc, radius, 0,360);
   }
}


//draws the features and fills its area
void draw_features(ezgl::renderer *g) {
    ezgl::rectangle visible_world = g->get_visible_world();

    for (size_t feat_id = 0; feat_id < features.size(); ++feat_id) {
        // Check if the feature intersects with the visible screen area
        if (!intersects(visible_world, features[feat_id].feature_rect)) {
            continue;
        }

        // Draw the feature
        switch (features[feat_id].type) {
            case LAKE:
                dark_mode_helper(g, DM_LAKE, LIGHT_BLUE);
                break;
            case RIVER:
            case STREAM:
                dark_mode_helper(g, DM_RIVER, LIGHT_BLUE);
                if(scale>2000){features[feat_id].show=false;}
                else{features[feat_id].show=true;}
                break;
            case PARK:
            case GREENSPACE:
            case GOLFCOURSE:
                dark_mode_helper(g, DARK_GREEN, LIGHT_GREEN);
                break;
            case ISLAND:
            case BEACH:
                dark_mode_helper(g, DM_SAND, SAND);
                break;
            case BUILDING:
                dark_mode_helper(g, DM_BUILDINGS, LIGHTER_GREY);
                if(scale>225){features[feat_id].show=false;}
                else{features[feat_id].show=true;}
                break;
            case GLACIER:
                dark_mode_helper(g, GREY, LIGHT_GREY);
                break;
            default:
                continue; // Skip unknown feature types
        }

        // Fill or draw the feature based on its type and scale
        if (features[feat_id].area != 0 && intersects(visible_world, features[feat_id].feature_rect)) {
            if(features[feat_id].show){
               g->fill_poly(features[feat_id].xy_locs);
            }
            
            
            
            
        } else if (features[feat_id].area == 0 && scale < 2500) {
            // Draw line features if it's a path and scale is appropriate
            for (int feat_point = 0; feat_point < (features[feat_id].num_feat_points) - 1; ++feat_point) {
                if (intersects(visible_world, {features[feat_id].xy_locs[feat_point], features[feat_id].xy_locs[feat_point + 1]})) {
                    g->draw_line(features[feat_id].xy_locs[feat_point], features[feat_id].xy_locs[feat_point + 1]);
                }
            }
        }
    }
}


//sets colour based on dark mode
void dark_mode_helper(ezgl::renderer *g, ezgl::color dark,ezgl::color light){
   if (dark_mode) {
      g->set_color(dark);
   } else {
      g->set_color(light);
   }
}

//decides which poi to dislay based on button toggle
void draw_POI(ezgl::renderer *g){ // ttc, food, gas stations, hotels, hospitals
   g->set_color(HOUSE_COLOR);

   if(scale<630){
      if (leisure_POIs == true){
         draw_POI_helper(g, leisure_pois, .04, leisure_icon);
      }
      if (food_POIs == true){
         draw_POI_helper(g, food_pois, .04, food_icon);
      }
      if (driving_POIs == true){
         draw_POI_helper(g, driving_pois, .04, driving_icon);
      }
      if (school_POIs == true){
        draw_POI_helper(g, school_pois, .04, school_icon);
      }
      if (emergency_POIs == true){
         draw_POI_helper(g, emergency_pois, .04, emergency_icon);
      }
      if (subway_station_POIs == true){
         draw_Detail_helper(g, osmSubwayStations, .04, subway_icon);
      }
   }
}

//draws the actual POIs
void draw_POI_helper(ezgl::renderer *g, std::vector<POI_data> poi_vector, float scaling_fac, ezgl::surface *icon){
   ezgl::rectangle visible_world = g->get_visible_world();
   
   for (size_t poi_id = 0; poi_id < poi_vector.size(); ++poi_id){
      poi_vector[poi_id].show = true;
      if(visible_world.contains(poi_vector[poi_id].xy_loc)){
         g->draw_surface(icon,poi_vector[poi_id].xy_loc,scaling_fac);
      }
   }
}
//draws the subway stations
void draw_Detail_helper(ezgl::renderer *g, std::vector<osm_data> osm_vector, float scaling_fac, ezgl::surface *icon){
   ezgl::rectangle visible_world = g->get_visible_world();
   
   for (size_t osm_id = 0; osm_id < osm_vector.size(); ++osm_id){
      osm_vector[osm_id].show = true;
      if(visible_world.contains(osm_vector[osm_id].xy_loc)){
         g->draw_surface(icon,osm_vector[osm_id].xy_loc,scaling_fac);
      }
   }
}

//draws the segments with/without curve points
void displaySeg(const seg_data& segment, ezgl::renderer *g, ezgl::color col, double width){
   const bool one_way_arrows = segment.one_way && scale < 75;
   g->set_line_width(width);
   g->set_color(col);

   if(!segment.xy_curve_points.empty()){
      //start to curve[0]
      ezgl::point2d prev = segment.start_xy;
      ezgl::point2d cur  = segment.xy_curve_points[0];
      displaySeg_helper(prev, cur, g, width);
      if(one_way_arrows) drawOneWays(g, prev, cur, segment);

      //draw the curve points
      for (size_t j = 0; j + 1 < segment.xy_curve_points.size(); ++j){
         prev = segment.xy_curve_points[j];
         cur  = segment.xy_curve_points[j+1];
         displaySeg_helper(prev, cur, g, width);
         if(one_way_arrows && j%3==0) drawOneWays(g, prev, cur, segment);
      }

      //draw curve[last] to end
      prev = segment.xy_curve_points.back();
      cur  = segment.end_xy;
      displaySeg_helper(prev, cur, g, width);
      if(one_way_arrows) drawOneWays(g, prev, cur, segment);
   }
   else{
      //draw start to end
      displaySeg_helper(segment.start_xy, segment.end_xy, g, width);
      if(one_way_arrows) drawOneWays(g, segment.start_xy, segment.end_xy, segment);
   }
}
//draws one sub-segment given pre-projected xy endpoints
void displaySeg_helper(const ezgl::point2d& start, const ezgl::point2d& end, ezgl::renderer *g, double width){
      // color and line width are set once by the caller; only cull + draw here.
      ezgl::rectangle seg_rect(start,end);
      if(intersects(g->get_visible_world(),seg_rect)){
         g->set_line_width(width);
         g->draw_line(start, end);
       }
}

//draws the street names
void draw_street_names(ezgl::renderer *g, const seg_data& segment){
   //skip invalid names before doing any work
   if(segment.name=="<unknown>" || segment.name.empty())
      return;
   //bound check first (cheapest reject)
   if(!intersects(g->get_visible_world(),segment.seg_rect))
      return;

   //use cached xy endpoints instead of re-projecting
   const double x1 = segment.start_xy.x;
   const double y1 = segment.start_xy.y;
   const double x2 = segment.end_xy.x;
   const double y2 = segment.end_xy.y;

   double orientation = 0;
   if(x2-x1 != 0){
      orientation = atan((y2-y1)/(x2-x1)) / kDegreeToRadian; // angle of rotation
   }

   g->set_color(dark_mode ? DM_STREET_NAME : ezgl::RED);
   g->set_font_size(10);
   //pick the correct font once (no redundant Arial double-set)
   if (china_font){
      g->format_font("Noto Sans CJK SC",ezgl::font_slant::normal,ezgl::font_weight::bold);
   } else if (iran_font){
      g->format_font("Noto Naskh Arabic",ezgl::font_slant::normal,ezgl::font_weight::bold);
   } else if (japan_font){
      g->format_font("Noto Sans CJK JP",ezgl::font_slant::normal,ezgl::font_weight::bold);
   } else {
      g->format_font("Arial",ezgl::font_slant::normal,ezgl::font_weight::bold);
   }
   g->set_text_rotation(orientation);
   g->draw_text({(x1+x2)/2,(y1+y2)/2}, segment.name, segment.length, segment.width*1.5);
}

//draws the one way arrows (endpoints already projected to xy)
void drawOneWays(ezgl::renderer *g, ezgl::point2d start_xy, ezgl::point2d end_xy, const seg_data& segment){
   //get angle
   double angle= atan2(end_xy.y-start_xy.y,end_xy.x-start_xy.x);
   double length=segment.width;

   end_xy.x=end_xy.x-(end_xy.x-start_xy.x)/2;
   end_xy.y=end_xy.y-(end_xy.y-start_xy.y)/2;


   ezgl::point2d top=ezgl::point2d(end_xy.x-length*cos(angle-M_PI/6),
                                     end_xy.y-length*sin(angle-M_PI/6));

   ezgl::point2d bottom=ezgl::point2d(end_xy.x-length*cos(angle+M_PI/6), 
                                       end_xy.y-length*sin(angle+M_PI/6));

   g->draw_line(end_xy, top);
   g->draw_line(end_xy, bottom);
}

//draws stop signs traffic lights,
void drawDetailOSM(ezgl::renderer *g){
   if(scale<26){
      //show stop signs and traffic lights
      draw_Detail_helper(g, osmStopSigns, .03, stop_sign_icon);
      draw_Detail_helper(g, osmTrafficLights, .04, traffic_light_icon);
      
   }
}

// Writes the nearest POI and intersection when map is clicked

// Check if 2 rectangles intersect
bool intersects(ezgl::rectangle r1, ezgl::rectangle r2){
   return !(r1.left() < r2.right()  && r1.right() > r2.left() &&
            r1.top() < r2.bottom()  && r1.bottom() > r2.top());

}


// Draws the route we need to take
void draw_highlighted_route(ezgl::renderer *g, const std::vector<StreetSegmentIdx>& highlighted_route, std::pair<IntersectionIdx, IntersectionIdx> intersect_pair){

   if(intersect_pair.first != -1 && intersect_pair.second != -1){
     
      intersections[intersect_pair.first].highlight = true;
      
      int radius=25;
      // Changes colour to black if not dark mode
      if(dark_mode == true){
         g->set_color(ezgl::WHITE);
      }else {
         g->set_color(ezgl::BLACK);
      } 
      ezgl::point2d inter_loc1 = intersections[intersect_pair.first].xy_loc;


      if(show_my_path && highlighted_route.size() > 0){
         g->fill_arc(inter_loc1, radius, 0,360);
         g->draw_surface(destination_icon,intersections[intersect_pair.second].xy_loc,.65);
         for(int i=0; i<highlighted_route.size(); i++){
               displaySeg(segments[highlighted_route[i]], g, HIGHLIGHTED_COL, 5);
               draw_street_names(g, segments[highlighted_route[i]]);
            }

      }


      
   }
}
