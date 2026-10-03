/* ui_state.h
   Shared declarations between the render-domain (map_renderer.cpp) and the
   UI-domain (map_ui.cpp) code that was split out of the original m2.cpp.

   The file-scope rendering/UI state variables are DEFINED exactly once in
   map_renderer.cpp and declared `extern` here so both translation units see
   the same objects (avoids duplicate-definition link errors).

   The function prototypes below are the forward-declaration block that used to
   sit at the top of m2.cpp; keeping them here lets either file call functions
   defined in the other. */

#ifndef UI_STATE_H
#define UI_STATE_H

#include "global.h"
#include <array>
#include <sstream>
#include <string>
#include <vector>

/* ---- forward declarations (moved verbatim from m2.cpp) ---- */
void draw_main_canvas(ezgl::renderer *g);
void act_on_mouse_click(ezgl::application* app, GdkEventButton* /*event*/, double x, double y);

void initial_setup (ezgl::application* application, bool /*new_window*/);
void toggle_clear (GtkWidget* /*widget*/, ezgl::application* application);
void toggle_find (GtkWidget* /*widget*/, ezgl::application* application);
void toggle_dark_mode (GtkWidget* /*widget*/, ezgl::application* application);
void toggle_leisure (GtkWidget* /*widget*/, ezgl::application* application);
void toggle_food (GtkWidget* /*widget*/, ezgl::application* application);
void toggle_driving (GtkWidget* /*widget*/, ezgl::application* application);
void toggle_school (GtkWidget* /*widget*/, ezgl::application* application);
void toggle_emergency (GtkWidget* /*widget*/, ezgl::application* application);
void toggle_subway_stations (GtkWidget* /*widget*/, ezgl::application* application);
void toggle_help (GtkWidget* /*widget*/, ezgl::application* application);
void combo_box_cbk(GtkComboBoxText* self, ezgl::application* app);

void draw_intersections(ezgl::renderer *);
void draw_segments(ezgl::renderer *);
void draw_highlighted_route(ezgl::renderer *g, const std::vector<StreetSegmentIdx>& highlighted_route,std::pair<IntersectionIdx, IntersectionIdx>);
void draw_dark_mode(ezgl::renderer *);
void drawDetailOSM(ezgl::renderer *);
void draw_Detail_helper(ezgl::renderer *, std::vector<osm_data>, float, ezgl::surface *);

void displaySeg_helper(const ezgl::point2d&, const ezgl::point2d&, ezgl::renderer *, double);
void draw_features(ezgl::renderer *);
void draw_street_names(ezgl::renderer *, const seg_data&);
void draw_feature_names(ezgl::renderer *,feature_data);
void draw_POI(ezgl::renderer *);
void draw_POI_helper(ezgl::renderer *, std::vector<POI_data>, float, ezgl::surface *);
void drawOneWays(ezgl::renderer *g, ezgl::point2d, ezgl::point2d, const seg_data&);
void on_entry_changed_1(GtkEntry *, gpointer);
void on_entry_changed_2(GtkEntry *, gpointer);
void on_entry_changed_3(GtkEntry *entry, gpointer user_data);
void on_entry_activated_3(GtkEntry *entry, ezgl::application* application);
// M3
void on_entry_changed_11(GtkEntry *, gpointer);
void on_entry_changed_22(GtkEntry *, gpointer);
//std::string determineTurnDirection(double angle);
void showDirections(const std::vector<StreetSegmentIdx> findPathBetweenIntersections);
std::string findCardinal(LatLon start_curr, LatLon end_curr);
std::string determineDirection(const std::string&, const std::string&);
void showErrorDialog(const std::stringstream& message);
// end of M3
void dark_mode_helper(ezgl::renderer *g, ezgl::color,ezgl::color);
std::pair<POI_data, POIIdx> closest_poi_finder(LatLon);
std::pair<POI_data, POIIdx> closest_poi_finder_helper(bool, std::vector<POI_data>, LatLon, std::pair<POI_data, POIIdx>);

/* prettyName is used by UI directions code (and is available to render if ever
   needed); defined once in map_ui.cpp. */
std::string prettyName(const std::string& raw);

/* ---- shared file-scope state (DEFINED in map_renderer.cpp) ---- */
extern std::vector<StreetSegmentIdx> my_path;
extern std::pair<IntersectionIdx, IntersectionIdx> my_path_start_end;

extern bool dark_mode;
extern bool leisure_POIs;
extern bool food_POIs;
extern bool driving_POIs;
extern bool school_POIs;
extern bool emergency_POIs;
extern bool subway_station_POIs;
extern bool help_popup;
extern bool show_my_path;

extern double worldWidth;
extern double worldHeight;
extern double screenWidth;
extern double screenHeight;
extern double scaleFactor;
extern double worldArea;
extern double scale;
extern double initialScale;
extern bool china_font;
extern bool iran_font;
extern bool japan_font;
extern ezgl::point2d center_world;
extern std::vector<std::string> list_of_directions;

extern std::pair<IntersectionIdx, IntersectionIdx> intersection_for_path;
extern std::string current_direction;
extern std::string next_direction;
extern std::stringstream dir;

#endif /* UI_STATE_H */
