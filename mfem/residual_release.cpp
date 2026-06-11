// Standalone MFEM residual-stress release solver.
//
// Unit convention:
//   length: mm
//   force:  N
//   stress: MPa = N/mm^2
//
// The weak form assembled by InitialStressHex3DLFIntegrator is
//   K u = - int_Omega B^T sigma0 dOmega
// for a linear elastic solid with an initial stress field sigma0.

#include "mfem.hpp"
#include "fem/picojson.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace mfem;
using namespace std;

namespace
{

string CurrentTimestamp()
{
   const auto now = chrono::system_clock::now();
   const time_t tt = chrono::system_clock::to_time_t(now);
   tm local_tm{};
#ifdef _WIN32
   localtime_s(&local_tm, &tt);
#else
   localtime_r(&tt, &local_tm);
#endif
   ostringstream os;
   os << put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
   return os.str();
}

void AppendProgress(const string &output_prefix, const string &message,
                    bool enabled)
{
   if (!enabled)
   {
      return;
   }
   ofstream out(string(output_prefix) + "_progress.log", ios::app);
   out << CurrentTimestamp() << "  " << message << endl;
}

struct Vec3
{
   real_t x = 0.0;
   real_t y = 0.0;
   real_t z = 0.0;
};

struct InitialStressModel
{
   string type = "zero";
   real_t sigma_xx_bottom_mpa = -120.0;
   real_t sigma_xx_top_mpa = 120.0;
   real_t sigma_xx_mpa = 0.0;
   real_t y_min_mm = -38.0;
   real_t height_mm = 38.0;
};

struct StressProfilePoint
{
   real_t r = 0.0;
   real_t sigma_xx_mpa = 0.0;
};

struct MachiningResidualStressModel
{
   bool enabled = false;
   bool sigma_xx_enabled = true;
   bool sigma_yy_enabled = false;
   bool sigma_zz_enabled = false;
   bool tau_xy_enabled = false;
   bool tau_xz_enabled = false;
   bool tau_yz_enabled = false;
   real_t layer_depth_mm = 0.30;
   vector<StressProfilePoint> profile;
};

struct MachiningStressEvent
{
   bool uses_contact_segment = false;
   Vec3 surface_p0_mm;
   Vec3 surface_p1_mm;
   Vec3 local_x = {1.0, 0.0, 0.0};
   Vec3 local_y = {0.0, 1.0, 0.0};
   Vec3 local_z = {0.0, 0.0, 1.0};
   real_t sweep_width_mm = -1.0;
   real_t layer_depth_mm = 0.0;

   // Legacy point-normal schema kept for old configs.
   Vec3 surface_point_mm;
   Vec3 surface_normal;
   Vec3 cutting_direction;
   real_t influence_radius_mm = -1.0;
   real_t sigma_cutting_surface_mpa = 0.0;
   real_t sigma_width_surface_mpa = 0.0;
   real_t sigma_normal_surface_mpa = 0.0;
};

struct ResidualStressConfig
{
   InitialStressModel initial;
   MachiningResidualStressModel machining;
   vector<MachiningStressEvent> machining_events;
};

struct SolverSummary
{
   int step = 0;
   real_t max_disp_mm = 0.0;
   real_t max_ux_mm = 0.0;
   real_t max_uy_mm = 0.0;
   real_t max_uz_mm = 0.0;
};

real_t Dot(const Vec3 &a, const Vec3 &b)
{
   return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Cross(const Vec3 &a, const Vec3 &b)
{
   return {a.y * b.z - a.z * b.y,
           a.z * b.x - a.x * b.z,
           a.x * b.y - a.y * b.x};
}

real_t Norm(const Vec3 &a)
{
   return sqrt(Dot(a, a));
}

Vec3 Normalize(const Vec3 &a, const Vec3 &fallback)
{
   const real_t n = Norm(a);
   if (n <= 1.0e-14)
   {
      return fallback;
   }
   return {a.x / n, a.y / n, a.z / n};
}

Vec3 operator-(const Vec3 &a, const Vec3 &b)
{
   return {a.x - b.x, a.y - b.y, a.z - b.z};
}

const picojson::object &AsObject(const picojson::value &v, const string &name)
{
   if (!v.is<picojson::object>())
   {
      throw runtime_error(name + " must be a JSON object.");
   }
   return v.get<picojson::object>();
}

bool HasKey(const picojson::object &obj, const string &key)
{
   return obj.find(key) != obj.end();
}

string GetString(const picojson::object &obj, const string &key,
                 const string &default_value)
{
   const auto it = obj.find(key);
   if (it == obj.end())
   {
      return default_value;
   }
   if (!it->second.is<string>())
   {
      throw runtime_error(key + " must be a string.");
   }
   return it->second.get<string>();
}

real_t GetNumber(const picojson::object &obj, const string &key,
                 real_t default_value)
{
   const auto it = obj.find(key);
   if (it == obj.end())
   {
      return default_value;
   }
   if (!it->second.is<double>())
   {
      throw runtime_error(key + " must be a number.");
   }
   return static_cast<real_t>(it->second.get<double>());
}

bool GetBool(const picojson::object &obj, const string &key,
             bool default_value)
{
   const auto it = obj.find(key);
   if (it == obj.end())
   {
      return default_value;
   }
   if (!it->second.is<bool>())
   {
      throw runtime_error(key + " must be a boolean.");
   }
   return it->second.get<bool>();
}

Vec3 GetVec3(const picojson::object &obj, const string &key,
             const Vec3 &default_value)
{
   const auto it = obj.find(key);
   if (it == obj.end())
   {
      return default_value;
   }
   if (!it->second.is<picojson::array>())
   {
      throw runtime_error(key + " must be an array with 3 numbers.");
   }
   const picojson::array &a = it->second.get<picojson::array>();
   if (a.size() != 3 || !a[0].is<double>() || !a[1].is<double>() ||
       !a[2].is<double>())
   {
      throw runtime_error(key + " must be an array with 3 numbers.");
   }
   return {static_cast<real_t>(a[0].get<double>()),
           static_cast<real_t>(a[1].get<double>()),
           static_cast<real_t>(a[2].get<double>())};
}

vector<StressProfilePoint> DefaultMachiningProfile()
{
   return {
      {0.00, -120.0}, {0.05, -193.0}, {0.10, -242.0},
      {0.15, -260.0}, {0.20, -245.0}, {0.25, -208.0},
      {0.30, -162.0}, {0.35, -120.0}, {0.40, -81.0},
      {0.45, -40.0},  {0.50, 1.0},    {0.55, 35.0},
      {0.60, 60.0},   {0.65, 78.0},   {0.70, 94.0},
      {0.75, 108.0},  {0.80, 117.0},  {0.85, 120.0},
      {0.90, 102.0},  {0.95, 58.0},   {1.00, 0.0}};
}

void SortAndValidateProfile(vector<StressProfilePoint> &profile)
{
   if (profile.empty())
   {
      throw runtime_error("machining_residual_stress.profile.points is empty.");
   }

   sort(profile.begin(), profile.end(),
        [](const StressProfilePoint &a, const StressProfilePoint &b)
        { return a.r < b.r; });

   for (size_t i = 0; i < profile.size(); ++i)
   {
      if (profile[i].r < 0.0 || profile[i].r > 1.0)
      {
         throw runtime_error(
            "machining_residual_stress.profile point r must be in [0, 1].");
      }
      if (i > 0 && profile[i].r <= profile[i - 1].r)
      {
         throw runtime_error(
            "machining_residual_stress.profile point r values must be unique.");
      }
   }
}

vector<StressProfilePoint> LoadStressProfilePoints(
   const picojson::object &profile_obj)
{
   const auto points_it = profile_obj.find("points");
   if (points_it == profile_obj.end())
   {
      return DefaultMachiningProfile();
   }
   if (!points_it->second.is<picojson::array>())
   {
      throw runtime_error("machining_residual_stress.profile.points must be an array.");
   }

   vector<StressProfilePoint> points;
   const picojson::array &json_points = points_it->second.get<picojson::array>();
   points.reserve(json_points.size());
   for (size_t i = 0; i < json_points.size(); ++i)
   {
      const picojson::object &point =
         AsObject(json_points[i], "machining_residual_stress.profile.points[" +
                                    to_string(i) + "]");
      StressProfilePoint p;
      p.r = GetNumber(point, "r", p.r);
      p.sigma_xx_mpa = GetNumber(point, "sigma_xx_mpa", p.sigma_xx_mpa);
      points.push_back(p);
   }
   SortAndValidateProfile(points);
   return points;
}

ResidualStressConfig LoadResidualStressConfig(const string &filename)
{
   ifstream in(filename);
   if (!in)
   {
      throw runtime_error("Cannot open stress config file: " + filename);
   }

   picojson::value root;
   const string err = picojson::parse(root, in);
   if (!err.empty())
   {
      throw runtime_error("JSON parse error in " + filename + ": " + err);
   }

   const picojson::object &root_obj = AsObject(root, "root");
   ResidualStressConfig config;
   config.machining.profile = DefaultMachiningProfile();
   const bool has_machining_control =
      HasKey(root_obj, "machining_residual_stress");

   if (HasKey(root_obj, "initial_stress"))
   {
      const picojson::object &initial =
         AsObject(root_obj.at("initial_stress"), "initial_stress");
      config.initial.type = GetString(initial, "type", config.initial.type);
      config.initial.sigma_xx_bottom_mpa =
         GetNumber(initial, "sigma_xx_bottom_mpa",
                   config.initial.sigma_xx_bottom_mpa);
      config.initial.sigma_xx_top_mpa =
         GetNumber(initial, "sigma_xx_top_mpa",
                   config.initial.sigma_xx_top_mpa);
      config.initial.sigma_xx_mpa =
         GetNumber(initial, "sigma_xx_mpa", config.initial.sigma_xx_mpa);
      config.initial.y_min_mm =
         GetNumber(initial, "y_min_mm", config.initial.y_min_mm);
      config.initial.height_mm =
         GetNumber(initial, "height_mm", config.initial.height_mm);
   }

   if (has_machining_control)
   {
      const picojson::object &machining =
         AsObject(root_obj.at("machining_residual_stress"),
                  "machining_residual_stress");
      config.machining.enabled =
         GetBool(machining, "enabled", config.machining.enabled);

      if (HasKey(machining, "stress_components"))
      {
         const picojson::object &components =
            AsObject(machining.at("stress_components"),
                     "machining_residual_stress.stress_components");
         config.machining.sigma_xx_enabled =
            GetBool(components, "sigma_xx_enabled",
                    config.machining.sigma_xx_enabled);
         config.machining.sigma_yy_enabled =
            GetBool(components, "sigma_yy_enabled",
                    config.machining.sigma_yy_enabled);
         config.machining.sigma_zz_enabled =
            GetBool(components, "sigma_zz_enabled",
                    config.machining.sigma_zz_enabled);
         config.machining.tau_xy_enabled =
            GetBool(components, "tau_xy_enabled",
                    config.machining.tau_xy_enabled);
         config.machining.tau_xz_enabled =
            GetBool(components, "tau_xz_enabled",
                    config.machining.tau_xz_enabled);
         config.machining.tau_yz_enabled =
            GetBool(components, "tau_yz_enabled",
                    config.machining.tau_yz_enabled);
      }

      if (HasKey(machining, "profile"))
      {
         const picojson::object &profile =
            AsObject(machining.at("profile"),
                     "machining_residual_stress.profile");
         config.machining.layer_depth_mm =
            GetNumber(profile, "layer_depth_mm",
                      config.machining.layer_depth_mm);
         config.machining.profile = LoadStressProfilePoints(profile);
      }
   }

   if (HasKey(root_obj, "machining_events"))
   {
      if (!root_obj.at("machining_events").is<picojson::array>())
      {
         throw runtime_error("machining_events must be an array.");
      }
      if (!has_machining_control)
      {
         config.machining.enabled = true;
      }
      const picojson::array &events =
         root_obj.at("machining_events").get<picojson::array>();
      config.machining_events.reserve(events.size());
      for (size_t i = 0; i < events.size(); ++i)
      {
         const picojson::object &event =
            AsObject(events[i], "machining_events[" + to_string(i) + "]");
         MachiningStressEvent e;
         e.layer_depth_mm =
            GetNumber(event, "layer_depth_mm",
                      config.machining.layer_depth_mm);

         if (HasKey(event, "surface_p0_mm") ||
             HasKey(event, "surface_p1_mm") ||
             HasKey(event, "local_frame"))
         {
            e.uses_contact_segment = true;
            if (!HasKey(event, "surface_p0_mm") ||
                !HasKey(event, "surface_p1_mm") ||
                !HasKey(event, "local_frame"))
            {
               throw runtime_error(
                  "contact machining_events require surface_p0_mm, "
                  "surface_p1_mm, and local_frame.");
            }
            e.surface_p0_mm =
               GetVec3(event, "surface_p0_mm", e.surface_p0_mm);
            e.surface_p1_mm =
               GetVec3(event, "surface_p1_mm", e.surface_p1_mm);
            e.sweep_width_mm =
               GetNumber(event, "sweep_width_mm", e.sweep_width_mm);

            const picojson::object &local_frame =
               AsObject(event.at("local_frame"),
                        "machining_events[" + to_string(i) + "].local_frame");
            e.local_x = GetVec3(local_frame, "x", e.local_x);
            e.local_y = GetVec3(local_frame, "y", e.local_y);
            e.local_z = GetVec3(local_frame, "z", e.local_z);
         }
         else
         {
            e.surface_point_mm =
               GetVec3(event, "surface_point_mm", e.surface_point_mm);
            e.surface_normal =
               GetVec3(event, "surface_normal", {0.0, 1.0, 0.0});
            e.cutting_direction =
               GetVec3(event, "cutting_direction", {1.0, 0.0, 0.0});
            e.influence_radius_mm =
               GetNumber(event, "influence_radius_mm", e.influence_radius_mm);
            e.sigma_cutting_surface_mpa =
               GetNumber(event, "sigma_cutting_surface_mpa",
                         e.sigma_cutting_surface_mpa);
            e.sigma_width_surface_mpa =
               GetNumber(event, "sigma_width_surface_mpa",
                         e.sigma_width_surface_mpa);
            e.sigma_normal_surface_mpa =
               GetNumber(event, "sigma_normal_surface_mpa",
                         e.sigma_normal_surface_mpa);
         }
         config.machining_events.push_back(e);
      }
   }

   if (config.initial.height_mm <= 0.0)
   {
      throw runtime_error("initial_stress.height_mm must be positive.");
   }
   if (config.machining.layer_depth_mm <= 0.0)
   {
      throw runtime_error(
         "machining_residual_stress.profile.layer_depth_mm must be positive.");
   }
   SortAndValidateProfile(config.machining.profile);

   return config;
}

void AddTensorToVoigt(const real_t tensor[3][3], Vector &s)
{
   s(0) += tensor[0][0];
   s(1) += tensor[1][1];
   s(2) += tensor[2][2];
   s(3) += tensor[0][1];
   s(4) += tensor[0][2];
   s(5) += tensor[1][2];
}

void EvaluateInitialStress(const InitialStressModel &model, const Vector &x,
                           Vector &s)
{
   if (model.type == "zero")
   {
      return;
   }
   if (model.type == "constant")
   {
      s(0) += model.sigma_xx_mpa;
      return;
   }

   const real_t y_rel = (x(1) - model.y_min_mm) / model.height_mm;
   if (model.type == "layered")
   {
      if (y_rel < 0.25)
      {
         s(0) += model.sigma_xx_bottom_mpa;
      }
      else if (y_rel > 0.75)
      {
         s(0) += model.sigma_xx_top_mpa;
      }
      return;
   }
   if (model.type == "linear_bending")
   {
      const real_t t = min<real_t>(1.0, max<real_t>(0.0, y_rel));
      s(0) += model.sigma_xx_bottom_mpa +
              t * (model.sigma_xx_top_mpa - model.sigma_xx_bottom_mpa);
      return;
   }

   throw runtime_error("Unknown initial_stress.type: " + model.type);
}

real_t EvaluateSigmaXxProfile(const MachiningResidualStressModel &model,
                              real_t r)
{
   if (r < 0.0 || r > 1.0 || model.profile.empty())
   {
      return 0.0;
   }

   if (r <= model.profile.front().r)
   {
      return model.profile.front().sigma_xx_mpa;
   }
   for (size_t i = 1; i < model.profile.size(); ++i)
   {
      const StressProfilePoint &left = model.profile[i - 1];
      const StressProfilePoint &right = model.profile[i];
      if (r <= right.r)
      {
         const real_t denom = right.r - left.r;
         if (denom <= 1.0e-14)
         {
            return right.sigma_xx_mpa;
         }
         const real_t t = (r - left.r) / denom;
         return left.sigma_xx_mpa +
                t * (right.sigma_xx_mpa - left.sigma_xx_mpa);
      }
   }
   return model.profile.back().sigma_xx_mpa;
}

void AddDirectionalSigmaXx(const Vec3 &local_x, real_t sigma_xx, Vector &s)
{
   const Vec3 x_axis = Normalize(local_x, {1.0, 0.0, 0.0});
   real_t tensor[3][3] = {};
   const real_t b[3] = {x_axis.x, x_axis.y, x_axis.z};
   for (int i = 0; i < 3; ++i)
   {
      for (int j = 0; j < 3; ++j)
      {
         tensor[i][j] = sigma_xx * b[i] * b[j];
      }
   }
   AddTensorToVoigt(tensor, s);
}

Vec3 OrthogonalizedAxis(const Vec3 &axis, const Vec3 &normal,
                        const Vec3 &fallback)
{
   const real_t an = Dot(axis, normal);
   return Normalize({axis.x - an * normal.x,
                     axis.y - an * normal.y,
                     axis.z - an * normal.z},
                    fallback);
}

void EvaluateContactSegmentMachiningStress(
   const MachiningResidualStressModel &model,
   const MachiningStressEvent &event, const Vector &x, Vector &s)
{
   if (event.layer_depth_mm <= 0.0)
   {
      return;
   }

   const Vec3 point{x(0), x(1), x(2)};
   const Vec3 z_axis = Normalize(event.local_z, {0.0, 0.0, 1.0});
   Vec3 x_axis = OrthogonalizedAxis(event.local_x, z_axis, {1.0, 0.0, 0.0});
   if (Norm(Cross(x_axis, z_axis)) <= 1.0e-12)
   {
      x_axis = OrthogonalizedAxis({1.0, 0.0, 0.0}, z_axis, {0.0, 1.0, 0.0});
   }

   Vec3 y_axis = OrthogonalizedAxis(event.local_y, z_axis, Cross(z_axis, x_axis));
   y_axis = OrthogonalizedAxis(y_axis, x_axis, Cross(z_axis, x_axis));
   const Vec3 y_ref = Normalize(Cross(z_axis, x_axis), {0.0, 1.0, 0.0});
   if (Dot(y_axis, y_ref) < 0.0)
   {
      y_axis = {-y_axis.x, -y_axis.y, -y_axis.z};
   }

   const Vec3 segment = event.surface_p1_mm - event.surface_p0_mm;
   const real_t segment_length = Norm(segment);
   if (segment_length <= 1.0e-12)
   {
      return;
   }

   const Vec3 dx = point - event.surface_p0_mm;
   const real_t depth = Dot(dx, z_axis);
   const real_t layer_depth =
      event.layer_depth_mm > 0.0 ? event.layer_depth_mm :
                                   model.layer_depth_mm;
   const real_t tol = max<real_t>(1.0e-8, layer_depth * 1.0e-6);
   if (depth < -tol || depth > layer_depth + tol)
   {
      return;
   }

   const real_t x_coord = Dot(dx, x_axis);
   if (event.sweep_width_mm > 0.0 &&
       fabs(x_coord) > event.sweep_width_mm + tol)
   {
      return;
   }

   const real_t y_coord = Dot(dx, y_axis);
   const real_t y1 = Dot(segment, y_axis);
   const real_t y_min = min<real_t>(0.0, y1) - tol;
   const real_t y_max = max<real_t>(0.0, y1) + tol;
   if (y_coord < y_min || y_coord > y_max)
   {
      return;
   }

   const real_t r = max<real_t>(0.0, min<real_t>(1.0, depth / layer_depth));
   const real_t sigma_xx = EvaluateSigmaXxProfile(model, r);
   AddDirectionalSigmaXx(x_axis, sigma_xx, s);
}

void EvaluateLegacyMachiningStress(const MachiningStressEvent &event,
                                   const Vector &x, Vector &s)
{
   if (event.layer_depth_mm <= 0.0)
   {
      return;
   }

   const Vec3 point{x(0), x(1), x(2)};
   const Vec3 n = Normalize(event.surface_normal, {0.0, 1.0, 0.0});
   Vec3 t = Normalize(event.cutting_direction, {1.0, 0.0, 0.0});

   const real_t tn = Dot(t, n);
   t = Normalize({t.x - tn * n.x, t.y - tn * n.y, t.z - tn * n.z},
                 {1.0, 0.0, 0.0});
   Vec3 w = Normalize(Cross(n, t), {0.0, 0.0, 1.0});

   const Vec3 dx = point - event.surface_point_mm;
   const real_t signed_distance = Dot(dx, n);
   const real_t depth = fabs(signed_distance);
   if (depth > 3.0 * event.layer_depth_mm)
   {
      return;
   }

   if (event.influence_radius_mm > 0.0)
   {
      const real_t normal_part = signed_distance;
      const Vec3 projected{dx.x - normal_part * n.x,
                           dx.y - normal_part * n.y,
                           dx.z - normal_part * n.z};
      if (Norm(projected) > event.influence_radius_mm)
      {
         return;
      }
   }

   const real_t decay = exp(-depth / event.layer_depth_mm);
   const real_t local[3] = {
      event.sigma_cutting_surface_mpa * decay,
      event.sigma_width_surface_mpa * decay,
      event.sigma_normal_surface_mpa * decay};
   const Vec3 basis[3] = {t, w, n};

   real_t tensor[3][3] = {};
   for (int a = 0; a < 3; ++a)
   {
      const real_t b[3] = {basis[a].x, basis[a].y, basis[a].z};
      for (int i = 0; i < 3; ++i)
      {
         for (int j = 0; j < 3; ++j)
         {
            tensor[i][j] += local[a] * b[i] * b[j];
         }
      }
   }

   AddTensorToVoigt(tensor, s);
}

void EvaluateMachiningStress(const MachiningResidualStressModel &model,
                             const MachiningStressEvent &event,
                             const Vector &x, Vector &s)
{
   if (event.uses_contact_segment)
   {
      if (!model.sigma_xx_enabled)
      {
         return;
      }
      EvaluateContactSegmentMachiningStress(model, event, x, s);
      return;
   }

   EvaluateLegacyMachiningStress(event, x, s);
}

void EvaluateResidualStress(const ResidualStressConfig &config, const Vector &x,
                            Vector &s)
{
   s.SetSize(6);
   s = 0.0;
   EvaluateInitialStress(config.initial, x, s);
   if (!config.machining.enabled)
   {
      return;
   }
   for (const MachiningStressEvent &event : config.machining_events)
   {
      EvaluateMachiningStress(config.machining, event, x, s);
   }
}

class InitialStressHex3DLFIntegrator : public LinearFormIntegrator
{
private:
   VectorCoefficient &sigma0_;
   DenseMatrix dshape_;
   DenseMatrix inv_jac_;
   DenseMatrix dshape_dx_;
   Vector sigma_;

public:
   explicit InitialStressHex3DLFIntegrator(VectorCoefficient &sigma0)
      : sigma0_(sigma0)
   {
      MFEM_VERIFY(sigma0_.GetVDim() == 6,
                  "sigma0 must be [sxx, syy, szz, sxy, sxz, syz].");
   }

   void AssembleRHSElementVect(const FiniteElement &el,
                               ElementTransformation &Tr,
                               Vector &elvect) override
   {
      MFEM_VERIFY(Tr.GetSpaceDim() == 3, "This integrator is 3D only.");
      MFEM_VERIFY(el.GetGeomType() == Geometry::CUBE,
                  "This integrator expects hexahedral CUBE elements.");

      const int dof = el.GetDof();
      elvect.SetSize(3 * dof);
      elvect = 0.0;

      dshape_.SetSize(dof, 3);
      inv_jac_.SetSize(3, 3);
      dshape_dx_.SetSize(dof, 3);
      sigma_.SetSize(6);

      const IntegrationRule *ir = IntRule;
      if (!ir)
      {
         const int int_order = 4;
         ir = &IntRules.Get(el.GetGeomType(), int_order);
      }

      for (int q = 0; q < ir->GetNPoints(); ++q)
      {
         const IntegrationPoint &ip = ir->IntPoint(q);
         Tr.SetIntPoint(&ip);

         el.CalcDShape(ip, dshape_);
         CalcInverse(Tr.Jacobian(), inv_jac_);
         Mult(dshape_, inv_jac_, dshape_dx_);

         sigma0_.Eval(sigma_, Tr, ip);
         const real_t w = ip.weight * Tr.Weight();

         for (int i = 0; i < dof; ++i)
         {
            const real_t dnx = dshape_dx_(i, 0);
            const real_t dny = dshape_dx_(i, 1);
            const real_t dnz = dshape_dx_(i, 2);

            elvect(i) -= w * (dnx * sigma_(0) + dny * sigma_(3) +
                              dnz * sigma_(4));
            elvect(i + dof) -= w * (dnx * sigma_(3) + dny * sigma_(1) +
                                    dnz * sigma_(5));
            elvect(i + 2 * dof) -= w * (dnx * sigma_(4) + dny * sigma_(5) +
                                        dnz * sigma_(2));
         }
      }
   }
};

Array<int> BuildEssentialBoundaryList(const Mesh &mesh, int fixed_attr,
                                      const FiniteElementSpace &fespace)
{
   if (mesh.bdr_attributes.Size() == 0)
   {
      throw runtime_error("Mesh has no boundary attributes.");
   }

   Array<int> ess_bdr(mesh.bdr_attributes.Max());
   ess_bdr = 0;
   if (fixed_attr < 1 || fixed_attr > ess_bdr.Size())
   {
      throw runtime_error("Requested boundary attribute is absent: " +
                          to_string(fixed_attr));
   }
   ess_bdr[fixed_attr - 1] = 1;

   Array<int> ess_tdof_list;
   fespace.GetEssentialTrueDofs(ess_bdr, ess_tdof_list);
   if (ess_tdof_list.Size() == 0)
   {
      throw runtime_error("Boundary attribute " + to_string(fixed_attr) +
                          " did not constrain any true DOFs.");
   }
   return ess_tdof_list;
}

SolverSummary ComputeSummary(const FiniteElementSpace &fespace,
                             const GridFunction &disp, int step)
{
   SolverSummary summary;
   summary.step = step;

   const int ndofs = fespace.GetNDofs();
   for (int i = 0; i < ndofs; ++i)
   {
      const real_t ux = disp(fespace.DofToVDof(i, 0));
      const real_t uy = disp(fespace.DofToVDof(i, 1));
      const real_t uz = disp(fespace.DofToVDof(i, 2));
      summary.max_ux_mm = max(summary.max_ux_mm, fabs(ux));
      summary.max_uy_mm = max(summary.max_uy_mm, fabs(uy));
      summary.max_uz_mm = max(summary.max_uz_mm, fabs(uz));
      summary.max_disp_mm =
         max(summary.max_disp_mm, sqrt(ux * ux + uy * uy + uz * uz));
   }

   return summary;
}

#ifdef MFEM_USE_MPI
SolverSummary ComputeSummaryParallel(const ParFiniteElementSpace &fespace,
                                     const ParGridFunction &disp, int step,
                                     MPI_Comm comm)
{
   SolverSummary local = ComputeSummary(fespace, disp, step);
   real_t local_values[4] = {local.max_disp_mm, local.max_ux_mm,
                             local.max_uy_mm, local.max_uz_mm};
   real_t global_values[4] = {0.0, 0.0, 0.0, 0.0};
   MPI_Allreduce(local_values, global_values, 4, MPITypeMap<real_t>::mpi_type,
                 MPI_MAX, comm);

   SolverSummary summary;
   summary.step = step;
   summary.max_disp_mm = global_values[0];
   summary.max_ux_mm = global_values[1];
   summary.max_uy_mm = global_values[2];
   summary.max_uz_mm = global_values[3];
   return summary;
}
#endif

string JsonEscape(const string &text)
{
   string escaped;
   escaped.reserve(text.size());
   for (char c : text)
   {
      switch (c)
      {
         case '\\': escaped += "\\\\"; break;
         case '"': escaped += "\\\""; break;
         case '\b': escaped += "\\b"; break;
         case '\f': escaped += "\\f"; break;
         case '\n': escaped += "\\n"; break;
         case '\r': escaped += "\\r"; break;
         case '\t': escaped += "\\t"; break;
         default: escaped += c; break;
      }
   }
   return escaped;
}

void SaveSummary(const string &filename, const SolverSummary &summary,
                 const string &mesh_file, const string &stress_config_file)
{
   ofstream out(filename);
   if (!out)
   {
      throw runtime_error("Cannot write summary file: " + filename);
   }
   out << fixed << setprecision(12);
   out << "{\n";
   out << "  \"step\": " << summary.step << ",\n";
   out << "  \"mesh_file\": \"" << JsonEscape(mesh_file) << "\",\n";
   out << "  \"stress_config_file\": \"" << JsonEscape(stress_config_file)
       << "\",\n";
   out << "  \"max_disp_mm\": " << summary.max_disp_mm << ",\n";
   out << "  \"max_ux_mm\": " << summary.max_ux_mm << ",\n";
   out << "  \"max_uy_mm\": " << summary.max_uy_mm << ",\n";
   out << "  \"max_uz_mm\": " << summary.max_uz_mm << "\n";
   out << "}\n";
}

void EnsureOutputDirectory(const string &output_prefix)
{
   const filesystem::path prefix(output_prefix);
   const filesystem::path parent = prefix.parent_path();
   if (!parent.empty())
   {
      filesystem::create_directories(parent);
   }
}

#ifdef _WIN32
string QuoteWindowsArgument(const string &arg)
{
   string quoted = "\"";
   for (char c : arg)
   {
      if (c == '"')
      {
         quoted += "\\\"";
      }
      else
      {
         quoted += c;
      }
   }
   quoted += "\"";
   return quoted;
}

string FindGLVisProgram()
{
   vector<filesystem::path> candidates;

   char module_path[MAX_PATH] = {0};
   if (GetModuleFileNameA(nullptr, module_path, MAX_PATH) > 0)
   {
      const filesystem::path exe_dir =
         filesystem::path(module_path).parent_path();
      candidates.push_back(exe_dir / "glvis" / "glvis.exe");
      candidates.push_back(exe_dir / "glvis.exe");
      candidates.push_back(exe_dir.parent_path() / "glvis" / "glvis.exe");
      candidates.push_back(exe_dir.parent_path() / "glvis.exe");
      candidates.push_back(exe_dir.parent_path() / "mfem" / "glvis" /
                           "glvis.exe");
      candidates.push_back(exe_dir.parent_path() / "mfem" / "glvis.exe");
   }

   const filesystem::path cwd = filesystem::current_path();
   candidates.push_back(cwd / "mfem" / "glvis" / "glvis.exe");
   candidates.push_back(cwd / "mfem" / "glvis.exe");
   candidates.push_back(cwd / "glvis" / "glvis.exe");
   candidates.push_back(cwd / "glvis.exe");
   candidates.push_back("E:/MFEM/glvis-windows/glvis.exe");

   for (const filesystem::path &candidate : candidates)
   {
      if (filesystem::exists(candidate))
      {
         return candidate.string();
      }
   }
   return "";
}

void LaunchGLVis(const string &mesh_file, const string &grid_function_file)
{
   if (!filesystem::exists(mesh_file))
   {
      cerr << "GLVis launch skipped; mesh file not found: " << mesh_file
           << endl;
      return;
   }
   if (!filesystem::exists(grid_function_file))
   {
      cerr << "GLVis launch skipped; gf file not found: "
           << grid_function_file << endl;
      return;
   }

   const string glvis = FindGLVisProgram();
   if (glvis.empty())
   {
      cerr << "GLVis launch skipped; glvis.exe was not found." << endl;
      return;
   }

   const string mesh_abs = filesystem::absolute(mesh_file).string();
   const string gf_abs = filesystem::absolute(grid_function_file).string();
   string command = QuoteWindowsArgument(glvis) + " -m " +
                    QuoteWindowsArgument(mesh_abs) + " -g " +
                    QuoteWindowsArgument(gf_abs);

   vector<char> command_line(command.begin(), command.end());
   command_line.push_back('\0');

   STARTUPINFOA startup_info;
   PROCESS_INFORMATION process_info;
   ZeroMemory(&startup_info, sizeof(startup_info));
   ZeroMemory(&process_info, sizeof(process_info));
   startup_info.cb = sizeof(startup_info);

   const string working_dir = filesystem::path(glvis).parent_path().string();
   const BOOL ok = CreateProcessA(nullptr, command_line.data(), nullptr,
                                  nullptr, FALSE, 0, nullptr,
                                  working_dir.c_str(), &startup_info,
                                  &process_info);
   if (!ok)
   {
      cerr << "Failed to launch GLVis. Command was: " << command << endl;
      return;
   }

   CloseHandle(process_info.hThread);
   CloseHandle(process_info.hProcess);
   cout << "GLVis launched for " << grid_function_file << endl;
}
#else
void LaunchGLVis(const string &, const string &)
{
   cerr << "Automatic GLVis launch is only implemented on Windows." << endl;
}
#endif

void PrintUsage(const char *program)
{
   cout << "Usage:\n"
        << "  " << program << " -m mesh.mesh --young 205000 --nu 0.3 "
        << "--bc-attr 1 --stress-config config.json --step 0 "
        << "--out data/residual/step_0000 [--visual-scale 1] "
        << "[-vis|-no-vis]\n";
}

} // namespace

int main(int argc, char *argv[])
{
#ifdef MFEM_USE_MPI
   Mpi::Init(argc, argv);
   const int num_procs = Mpi::WorldSize();
   const int myid = Mpi::WorldRank();
   Hypre::Init();
#else
   const int num_procs = 1;
   const int myid = 0;
#endif

   const char *mesh_file = "";
   const char *stress_config_file = "";
   const char *output_prefix = "data/residual/step_0000";
   int fixed_boundary_attr = 1;
   int step = 0;
   int order = 1;
   real_t young = 205000.0;
   real_t poisson = 0.30;
   real_t visual_scale = 1.0;
   bool visualization = true;

   OptionsParser args(argc, argv);
   args.AddOption(&mesh_file, "-m", "--mesh", "Input MFEM mesh file.");
   args.AddOption(&young, "--young", "--young-modulus",
                  "Young's modulus in MPa = N/mm^2.");
   args.AddOption(&poisson, "--nu", "--poisson", "Poisson ratio.");
   args.AddOption(&fixed_boundary_attr, "--bc-attr",
                  "--fixed-boundary-attr",
                  "Boundary attribute fixed by essential BCs.");
   args.AddOption(&stress_config_file, "--stress-config",
                  "--stress-config-file", "Residual stress JSON config.");
   args.AddOption(&step, "--step", "--step-id", "Machining step id.");
   args.AddOption(&output_prefix, "--out", "--output-prefix",
                  "Output prefix without extension.");
   args.AddOption(&order, "--order", "--order",
                  "H1 displacement finite element order.");
   args.AddOption(&visual_scale, "--visual-scale", "--visualization-scale",
                  "Scale applied only to the deformed mesh output.");
   args.AddOption(&visualization, "-vis", "--visualization", "-no-vis",
                  "--no-visualization",
                  "Launch GLVis immediately after writing the displacement gf.");
   args.Parse();

   if (!args.Good())
   {
      if (myid == 0)
      {
         args.PrintUsage(cout);
      }
      return 1;
   }
   if (string(mesh_file).empty() || string(stress_config_file).empty())
   {
      if (myid == 0)
      {
         PrintUsage(argv[0]);
      }
      return 1;
   }

   try
   {
#ifdef MFEM_USE_MPI
      const bool is_root = (myid == 0);
      if (is_root)
      {
         EnsureOutputDirectory(output_prefix);
      }
      MPI_Barrier(MPI_COMM_WORLD);
      if (is_root)
      {
         ofstream(string(output_prefix) + "_progress.log", ios::trunc)
            << CurrentTimestamp() << "  residual_release started. ranks="
            << num_procs << ", step=" << step << endl;
      }

      AppendProgress(output_prefix, "Loading residual stress config.",
                     is_root);
      if (is_root)
      {
         cout << "Loading residual stress config: " << stress_config_file
              << endl;
      }
      ResidualStressConfig config =
         LoadResidualStressConfig(stress_config_file);
      if (is_root)
      {
         cout << "Residual stress config loaded. machining_events="
              << config.machining_events.size() << endl;
         cout << "Loading mesh: " << mesh_file << endl;
      }
      AppendProgress(output_prefix,
                     "Residual stress config loaded. machining_events=" +
                        to_string(config.machining_events.size()) +
                        ". Loading mesh.",
                     is_root);

      Mesh serial_mesh(mesh_file, 1, 1);
      const int dim = serial_mesh.Dimension();
      if (dim != 3)
      {
         throw runtime_error("Only 3D meshes are supported.");
      }

      ParMesh mesh(MPI_COMM_WORLD, serial_mesh);
      int local_elements = mesh.GetNE();
      vector<int> element_counts;
      if (is_root)
      {
         element_counts.resize(num_procs);
      }
      MPI_Gather(&local_elements, 1, MPI_INT,
                 is_root ? element_counts.data() : nullptr, 1, MPI_INT,
                 0, MPI_COMM_WORLD);
      if (is_root)
      {
         ostringstream os;
         os << "Mesh loaded. serial_elements=" << serial_mesh.GetNE()
            << ", local_elements_by_rank=[";
         for (int i = 0; i < num_procs; ++i)
         {
            if (i > 0)
            {
               os << ",";
            }
            os << element_counts[i];
         }
         os << "]";
         AppendProgress(output_prefix, os.str(), true);
      }
      H1_FECollection fec(order, dim);
      ParFiniteElementSpace fespace(&mesh, &fec, dim);
      AppendProgress(output_prefix,
                     "Finite element space ready. true_vsize=" +
                        to_string(fespace.GlobalTrueVSize()),
                     is_root);

      Array<int> ess_tdof_list =
         BuildEssentialBoundaryList(mesh, fixed_boundary_attr, fespace);
      AppendProgress(output_prefix,
                     "Essential boundary list ready. local_size=" +
                        to_string(ess_tdof_list.Size()),
                     is_root);

      const real_t lambda =
         young * poisson / ((1.0 + poisson) * (1.0 - 2.0 * poisson));
      const real_t mu = young / (2.0 * (1.0 + poisson));
      ConstantCoefficient lambda_coeff(lambda);
      ConstantCoefficient mu_coeff(mu);

      if (is_root)
      {
         cout << "Assembling stiffness matrix..." << endl;
      }
      AppendProgress(output_prefix, "Assembling stiffness matrix.", is_root);
      ParBilinearForm a(&fespace);
      a.AddDomainIntegrator(new ElasticityIntegrator(lambda_coeff, mu_coeff));
      a.Assemble();
      AppendProgress(output_prefix, "Stiffness matrix assembled.", is_root);

      VectorFunctionCoefficient sigma0_coeff(
         6, [&config](const Vector &x, Vector &s)
         { EvaluateResidualStress(config, x, s); });

      if (is_root)
      {
         cout << "Assembling residual load..." << endl;
      }
      AppendProgress(output_prefix, "Assembling residual load.", is_root);
      ParLinearForm b(&fespace);
      b.AddDomainIntegrator(new InitialStressHex3DLFIntegrator(sigma0_coeff));
      b.Assemble();
      AppendProgress(output_prefix, "Residual load assembled.", is_root);

      ParGridFunction disp(&fespace);
      disp = 0.0;

      HypreParMatrix A;
      Vector B, X;
      AppendProgress(output_prefix, "Forming linear system.", is_root);
      a.FormLinearSystem(ess_tdof_list, disp, b, A, X, B);
      AppendProgress(output_prefix,
                     "Linear system formed. global_rows=" +
                        to_string(A.GetGlobalNumRows()),
                     is_root);

      if (is_root)
      {
         cout << "Residual release MPI ranks: " << num_procs
              << ", global system size: " << A.GetGlobalNumRows() << endl;
      }

      HypreBoomerAMG preconditioner(A);
      preconditioner.SetElasticityOptions(&fespace);
      preconditioner.SetPrintLevel(0);

      HyprePCG pcg(A);
      pcg.SetTol(1.0e-6);
      pcg.SetAbsTol(0.0);
      pcg.SetMaxIter(300);
      pcg.SetPrintLevel(1);
      pcg.SetPreconditioner(preconditioner);
      if (is_root)
      {
         cout << "Solving residual release system with HyprePCG..." << endl;
      }
      AppendProgress(output_prefix, "Solving with HyprePCG.", is_root);
      pcg.Mult(B, X);
      AppendProgress(output_prefix, "HyprePCG finished.", is_root);

      a.RecoverFEMSolution(X, b, disp);
      AppendProgress(output_prefix, "FEM solution recovered.", is_root);

      const string solution_mesh_file = string(output_prefix) + "_mesh.mesh";
      const string disp_file = string(output_prefix) + "_disp.gf";
      const string deformed_mesh_file =
         string(output_prefix) + "_deformed.mesh";
      const string summary_file = string(output_prefix) + "_summary.json";

      if (is_root)
      {
         cout << "Saving residual release outputs..." << endl;
      }
      AppendProgress(output_prefix, "Saving mesh and displacement.", is_root);
      mesh.SetNodalFESpace(&fespace);
      mesh.SaveAsOne(solution_mesh_file, 12);
      disp.SaveAsOne(disp_file.c_str(), 12);
      AppendProgress(output_prefix, "Mesh and displacement saved.", is_root);

      SolverSummary summary =
         ComputeSummaryParallel(fespace, disp, step, MPI_COMM_WORLD);
      AppendProgress(output_prefix, "Summary computed.", is_root);
      if (is_root)
      {
         SaveSummary(summary_file, summary, solution_mesh_file,
                     stress_config_file);
         AppendProgress(output_prefix, "Summary saved.", true);
         if (visualization)
         {
            LaunchGLVis(solution_mesh_file, disp_file);
            AppendProgress(output_prefix, "GLVis launch requested.", true);
         }
      }

      GridFunction *nodes = mesh.GetNodes();
      if (!nodes)
      {
         throw runtime_error("Failed to access mesh nodes for output.");
      }
      ParGridFunction disp_vis = disp;
      disp_vis *= visual_scale;
      *nodes += disp_vis;
      mesh.SaveAsOne(deformed_mesh_file, 12);
      AppendProgress(output_prefix, "Deformed mesh saved.", is_root);

      if (is_root)
      {
         cout << fixed << setprecision(8)
              << "Residual release solved. step=" << step
              << ", max_disp_mm=" << summary.max_disp_mm << endl;
         AppendProgress(output_prefix, "Residual release solved.", true);
      }
#else
      EnsureOutputDirectory(output_prefix);

      ResidualStressConfig config =
         LoadResidualStressConfig(stress_config_file);

      Mesh mesh(mesh_file, 1, 1);
      const int dim = mesh.Dimension();
      if (dim != 3)
      {
         throw runtime_error("Only 3D meshes are supported.");
      }

      H1_FECollection fec(order, dim);
      FiniteElementSpace fespace(&mesh, &fec, dim);

      Array<int> ess_tdof_list =
         BuildEssentialBoundaryList(mesh, fixed_boundary_attr, fespace);

      const real_t lambda =
         young * poisson / ((1.0 + poisson) * (1.0 - 2.0 * poisson));
      const real_t mu = young / (2.0 * (1.0 + poisson));
      ConstantCoefficient lambda_coeff(lambda);
      ConstantCoefficient mu_coeff(mu);

      BilinearForm a(&fespace);
      a.AddDomainIntegrator(new ElasticityIntegrator(lambda_coeff, mu_coeff));
      a.Assemble();

      VectorFunctionCoefficient sigma0_coeff(
         6, [&config](const Vector &x, Vector &s)
         { EvaluateResidualStress(config, x, s); });

      LinearForm b(&fespace);
      b.AddDomainIntegrator(new InitialStressHex3DLFIntegrator(sigma0_coeff));
      b.Assemble();

      GridFunction disp(&fespace);
      disp = 0.0;

      OperatorPtr A;
      Vector B, X;
      a.FormLinearSystem(ess_tdof_list, disp, b, A, X, B);

      GSSmoother preconditioner(static_cast<SparseMatrix &>(*A));
      PCG(*A, preconditioner, B, X, 1, 1000, 1.0e-12, 0.0);
      a.RecoverFEMSolution(X, b, disp);

      const string disp_file = string(output_prefix) + "_disp.gf";
      const string deformed_mesh_file =
         string(output_prefix) + "_deformed.mesh";
      const string summary_file = string(output_prefix) + "_summary.json";

      {
         ofstream out(disp_file);
         out.precision(12);
         disp.Save(out);
      }
      if (visualization)
      {
         LaunchGLVis(mesh_file, disp_file);
      }

      SolverSummary summary = ComputeSummary(fespace, disp, step);
      SaveSummary(summary_file, summary, mesh_file, stress_config_file);

      mesh.SetNodalFESpace(&fespace);
      GridFunction *nodes = mesh.GetNodes();
      if (!nodes)
      {
         throw runtime_error("Failed to access mesh nodes for output.");
      }
      GridFunction disp_vis = disp;
      disp_vis *= visual_scale;
      *nodes += disp_vis;
      {
         ofstream out(deformed_mesh_file);
         out.precision(12);
         mesh.Print(out);
      }

      cout << fixed << setprecision(8)
           << "Residual release solved. step=" << step
           << ", max_disp_mm=" << summary.max_disp_mm << endl;
#endif
   }
   catch (const exception &e)
   {
      if (myid == 0)
      {
         cerr << "residual_release failed: " << e.what() << endl;
      }
#ifdef MFEM_USE_MPI
      MPI_Abort(MPI_COMM_WORLD, 2);
#endif
      return 2;
   }

   return 0;
}
