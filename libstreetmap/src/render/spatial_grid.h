/*
 * Uniform spatial grid over the map's xy world bounds.
 *
 * The draw loop used to scan every segment and every feature each frame and
 * reject the ones outside the view with a rectangle test — O(N) per frame
 * regardless of zoom. This grid buckets items by the cells their bounding
 * rectangle overlaps, so a frame only visits items in the visible cells.
 *
 * Items are referenced by index into the global `segments` / `features`
 * vectors. An item whose rect spans several cells is stored in each; a
 * per-query "epoch" stamp guarantees each index is yielded at most once per
 * query without clearing state between frames.
 */
#ifndef SPATIAL_GRID_H
#define SPATIAL_GRID_H

#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include "ezgl/rectangle.hpp"

class SpatialGrid {
public:
   SpatialGrid() = default;

   // Configure the grid to cover [min_x,max_x] x [min_y,max_y] with roughly
   // `target_cells` buckets, then (re)allocate empty cells.
   void init(double minx, double miny, double maxx, double maxy, int target_cells) {
      min_x_ = minx; min_y_ = miny;
      double w = std::max(1e-9, maxx - minx);
      double h = std::max(1e-9, maxy - miny);
      // Keep cells roughly square and the count near target_cells.
      double aspect = w / h;
      int rows = std::max(1, (int)std::round(std::sqrt(target_cells / std::max(1e-9, aspect))));
      int cols = std::max(1, (int)std::round((double)target_cells / rows));
      cols_ = cols; rows_ = rows;
      inv_cell_w_ = cols_ / w;
      inv_cell_h_ = rows_ / h;
      cells_.assign((size_t)cols_ * rows_, {});
      epoch_.assign(0, 0); // reset; sized lazily in query
      query_epoch_ = 0;
   }

   bool ready() const { return !cells_.empty(); }

   // Insert item `idx` (index into the owning vector) covering world rect r.
   void insert(int idx, const ezgl::rectangle& r) {
      int c0, r0, c1, r1;
      cellRange(r, c0, r0, c1, r1);
      for (int ry = r0; ry <= r1; ++ry)
         for (int cx = c0; cx <= c1; ++cx)
            cells_[(size_t)ry * cols_ + cx].push_back(idx);
   }

   // Call out[] with each unique item index whose cell overlaps `view`.
   // `n_items` is the size of the owning vector (used to size the epoch map).
   template <typename Fn>
   void query(const ezgl::rectangle& view, int n_items, Fn&& visit) {
      if (cells_.empty()) return;
      if ((int)epoch_.size() < n_items) epoch_.assign(n_items, 0);
      ++query_epoch_;
      int c0, r0, c1, r1;
      cellRange(view, c0, r0, c1, r1);
      for (int ry = r0; ry <= r1; ++ry) {
         for (int cx = c0; cx <= c1; ++cx) {
            for (int idx : cells_[(size_t)ry * cols_ + cx]) {
               if (epoch_[idx] == query_epoch_) continue; // already visited
               epoch_[idx] = query_epoch_;
               visit(idx);
            }
         }
      }
   }

   void clear() {
      cells_.clear();
      epoch_.clear();
      query_epoch_ = 0;
   }

private:
   void cellRange(const ezgl::rectangle& r, int& c0, int& r0, int& c1, int& r1) const {
      c0 = clampCol((int)std::floor((r.left()   - min_x_) * inv_cell_w_));
      c1 = clampCol((int)std::floor((r.right()  - min_x_) * inv_cell_w_));
      r0 = clampRow((int)std::floor((r.bottom() - min_y_) * inv_cell_h_));
      r1 = clampRow((int)std::floor((r.top()    - min_y_) * inv_cell_h_));
      if (c1 < c0) std::swap(c0, c1);
      if (r1 < r0) std::swap(r0, r1);
   }
   int clampCol(int c) const { return std::min(std::max(c, 0), cols_ - 1); }
   int clampRow(int r) const { return std::min(std::max(r, 0), rows_ - 1); }

   int cols_ = 0, rows_ = 0;
   double min_x_ = 0, min_y_ = 0;
   double inv_cell_w_ = 0, inv_cell_h_ = 0;
   std::vector<std::vector<int>> cells_;
   std::vector<std::uint32_t> epoch_;
   std::uint32_t query_epoch_ = 0;
};

// Global grids over the segment and feature arrays, built at load time.
extern SpatialGrid g_segment_grid;
extern SpatialGrid g_feature_grid;

// Builds both grids from the loaded `segments` and `features` vectors.
void buildSpatialGrids();

#endif /* SPATIAL_GRID_H */
