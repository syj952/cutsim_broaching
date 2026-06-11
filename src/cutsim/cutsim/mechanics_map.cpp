#include "mechanics_map.hpp"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>

namespace cutsim {
    namespace {
        constexpr std::size_t npos = std::numeric_limits<std::size_t>::max();

        void writeVertex(std::ostream& out, const GLVertex& value)
        {
            out << value.x << ' ' << value.y << ' ' << value.z << ' ';
        }

        bool readVertex(std::istream& in, GLVertex& value)
        {
            return static_cast<bool>(in >> value.x >> value.y >> value.z);
        }

        void writeRecord(std::ostream& out, const MechanicsMapRecord& record)
        {
            out << "record "
                << record.angle_index << ' '
                << record.tool_angle << ' '
                << record.blade_id << ' '
                << record.inside_index << ' '
                << (record.active ? 1 : 0) << ' '
                << record.h0 << ' '
                << record.h_online << ' '
                << record.w << ' '
                << record.force_position_id << ' '
                << record.node_id_bottom << ' '
                << record.node_id_up << ' ';

            writeVertex(out, record.P0);
            writeVertex(out, record.P1);
            writeVertex(out, record.Q);
            writeVertex(out, record.n_h);
            writeVertex(out, record.vr);
            writeVertex(out, record.vt);
            writeVertex(out, record.va);
            writeVertex(out, record.nominal_force);

            for (double value : record.Phi0) {
                out << value << ' ';
            }
            for (double value : record.dPhi_dh) {
                out << value << ' ';
            }
            for (double value : record.Phi_online) {
                out << value << ' ';
            }
            for (double value : record.S_q) {
                out << value << ' ';
            }
            for (double value : record.B_force) {
                out << value << ' ';
            }
            out << '\n';
        }
    }

    void MechanicsMapLibrary::clear()
    {
        std::vector<MechanicsMapRecord>().swap(records_);
        modal_count = 0;
        angle_step = 0.0;
        theta_f = {};
    }

    bool MechanicsMapLibrary::empty() const
    {
        return records_.empty();
    }

    std::size_t MechanicsMapLibrary::size() const
    {
        return records_.size();
    }

    const std::vector<MechanicsMapRecord>& MechanicsMapLibrary::records() const
    {
        return records_;
    }

    std::size_t MechanicsMapLibrary::findRecordIndex(const MechanicsMapRecord& record) const
    {
        for (std::size_t i = 0; i < records_.size(); ++i) {
            const MechanicsMapRecord& current = records_[i];
            if (current.angle_index == record.angle_index &&
                current.blade_id == record.blade_id &&
                current.inside_index == record.inside_index) {
                return i;
            }
        }
        return npos;
    }

    void MechanicsMapLibrary::addOrReplace(const MechanicsMapRecord& record)
    {
        const std::size_t index = findRecordIndex(record);
        if (index == npos) {
            records_.push_back(record);
        }
        else {
            records_[index] = record;
        }
        modal_count = std::max(modal_count, record.S_q.size());
    }

    bool MechanicsMapLibrary::writeHeader(std::ostream& out, std::size_t record_count, std::string* error) const
    {
        out << "XCUTSIM_MECHANICS_MAP 1\n";
        out << "angle_step " << angle_step << '\n';
        out << "modal_count " << modal_count << '\n';
        out << "theta_f ";
        for (double value : theta_f) {
            out << value << ' ';
        }
        out << '\n';
        out << "records " << record_count << '\n';

        if (!out) {
            if (error) {
                *error = "failed to write mechanics map header";
            }
            return false;
        }
        return true;
    }

    bool MechanicsMapLibrary::writeRecords(std::ostream& out, std::string* error) const
    {
        for (const MechanicsMapRecord& record : records_) {
            writeRecord(out, record);
        }

        if (!out) {
            if (error) {
                *error = "failed to write mechanics map records";
            }
            return false;
        }
        return true;
    }

    void MechanicsMapLibrary::removeForAngleBlade(int angle_index, int blade_id)
    {
        records_.erase(
            std::remove_if(records_.begin(), records_.end(),
                [angle_index, blade_id](const MechanicsMapRecord& record) {
                    return record.angle_index == angle_index && record.blade_id == blade_id;
                }),
            records_.end());
    }

    std::vector<const MechanicsMapRecord*> MechanicsMapLibrary::recordsForAngle(int angle_index) const
    {
        std::vector<const MechanicsMapRecord*> result;
        for (const MechanicsMapRecord& record : records_) {
            if (record.angle_index == angle_index) {
                result.push_back(&record);
            }
        }
        return result;
    }

    bool MechanicsMapLibrary::saveToFile(const std::string& path, std::string* error) const
    {
        try {
            const std::filesystem::path output_path(path);
            if (output_path.has_parent_path()) {
                std::filesystem::create_directories(output_path.parent_path());
            }

            std::ofstream out(path);
            if (!out.is_open()) {
                if (error) {
                    *error = "failed to open mechanics map file for writing";
                }
                return false;
            }

            out << std::setprecision(17);
            return writeHeader(out, records_.size(), error) && writeRecords(out, error);
        }
        catch (const std::exception& ex) {
            if (error) {
                *error = ex.what();
            }
            return false;
        }
    }

    bool MechanicsMapLibrary::loadFromFile(const std::string& path, std::string* error)
    {
        std::ifstream in(path);
        if (!in.is_open()) {
            if (error) {
                *error = "failed to open mechanics map file for reading";
            }
            return false;
        }

        clear();

        std::string tag;
        int version = 0;
        if (!(in >> tag >> version) || tag != "XCUTSIM_MECHANICS_MAP" || version != 1) {
            if (error) {
                *error = "invalid mechanics map file header";
            }
            return false;
        }

        std::size_t expected_records = 0;
        while (in >> tag) {
            if (tag == "angle_step") {
                in >> angle_step;
            }
            else if (tag == "modal_count") {
                in >> modal_count;
            }
            else if (tag == "theta_f") {
                for (double& value : theta_f) {
                    in >> value;
                }
            }
            else if (tag == "records") {
                in >> expected_records;
                records_.reserve(expected_records);
            }
            else if (tag == "record") {
                MechanicsMapRecord record;
                int active = 0;
                in >> record.angle_index
                    >> record.tool_angle
                    >> record.blade_id
                    >> record.inside_index
                    >> active
                    >> record.h0
                    >> record.h_online
                    >> record.w
                    >> record.force_position_id
                    >> record.node_id_bottom
                    >> record.node_id_up;
                record.active = active != 0;

                if (!readVertex(in, record.P0) ||
                    !readVertex(in, record.P1) ||
                    !readVertex(in, record.Q) ||
                    !readVertex(in, record.n_h) ||
                    !readVertex(in, record.vr) ||
                    !readVertex(in, record.vt) ||
                    !readVertex(in, record.va) ||
                    !readVertex(in, record.nominal_force)) {
                    if (error) {
                        *error = "invalid mechanics map vector field";
                    }
                    return false;
                }

                for (double& value : record.Phi0) {
                    in >> value;
                }
                for (double& value : record.dPhi_dh) {
                    in >> value;
                }
                for (double& value : record.Phi_online) {
                    in >> value;
                }

                record.S_q.resize(modal_count, 0.0);
                record.B_force.resize(modal_count * 3, 0.0);
                for (double& value : record.S_q) {
                    in >> value;
                }
                for (double& value : record.B_force) {
                    in >> value;
                }

                if (!in) {
                    if (error) {
                        *error = "truncated mechanics map record";
                    }
                    return false;
                }

                addOrReplace(record);
            }
            else {
                if (error) {
                    *error = "unknown mechanics map token: " + tag;
                }
                return false;
            }
        }

        return true;
    }

    const char* defaultMechanicsMapPath()
    {
        return "data/mechanics_map_library.txt";
    }

} // namespace cutsim
