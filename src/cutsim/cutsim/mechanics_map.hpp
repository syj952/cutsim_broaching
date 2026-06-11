#ifndef MECHANICS_MAP_HPP
#define MECHANICS_MAP_HPP

#include <array>
#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

#include "glvertex.hpp"

namespace cutsim {

    struct MechanicsMapRecord {
        int angle_index = 0;
        double tool_angle = 0.0;
        int blade_id = 0;
        int inside_index = 0;
        bool active = false;

        double h0 = 0.0;
        double h_online = 0.0;
        double w = 0.0;

        int force_position_id = -1;
        int node_id_bottom = -1;
        int node_id_up = -1;

        GLVertex P0;
        GLVertex P1;
        GLVertex Q;
        GLVertex n_h;

        GLVertex vr;
        GLVertex vt;
        GLVertex va;
        GLVertex nominal_force;

        std::array<double, 18> Phi0{};
        std::array<double, 18> dPhi_dh{};
        std::array<double, 18> Phi_online{};
        std::vector<double> S_q;
        std::vector<double> B_force;
    };

    class MechanicsMapLibrary {
    public:
        double angle_step = 0.0;
        std::size_t modal_count = 0;
        std::array<double, 6> theta_f{};

        void clear();
        bool empty() const;
        std::size_t size() const;

        void addOrReplace(const MechanicsMapRecord& record);
        void removeForAngleBlade(int angle_index, int blade_id);
        const std::vector<MechanicsMapRecord>& records() const;
        std::vector<const MechanicsMapRecord*> recordsForAngle(int angle_index) const;

        bool saveToFile(const std::string& path, std::string* error = nullptr) const;
        bool loadFromFile(const std::string& path, std::string* error = nullptr);
        bool writeHeader(std::ostream& out, std::size_t record_count, std::string* error = nullptr) const;
        bool writeRecords(std::ostream& out, std::string* error = nullptr) const;

    private:
        std::vector<MechanicsMapRecord> records_;
        std::size_t findRecordIndex(const MechanicsMapRecord& record) const;
    };

    const char* defaultMechanicsMapPath();

} // namespace cutsim

#endif
