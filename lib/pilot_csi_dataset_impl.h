/*
 * Copyright 2025
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_IEEE802_11_PILOT_CSI_DATASET_IMPL_H
#define INCLUDED_IEEE802_11_PILOT_CSI_DATASET_IMPL_H

#include <ieee802_11/pilot_csi_dataset.h>

#include <gnuradio/gr_complex.h>

#include <fstream>
#include <mutex>
#include <vector>

namespace gr {
namespace ieee802_11 {

class pilot_csi_dataset_impl : public pilot_csi_dataset
{
public:
    pilot_csi_dataset_impl(const std::string& filename, bool append);
    ~pilot_csi_dataset_impl();

private:
    void handle_pilots(pmt::pmt_t msg);
    void handle_csi(pmt::pmt_t msg);
    void maybe_write_locked();
    void write_header_locked();

    std::mutex d_mutex;
    std::ofstream d_file;
    bool d_append;
    bool d_header_written;
    bool d_has_pilots;
    bool d_has_csi;
    uint64_t d_seq;
    std::vector<gr_complex> d_pilots;
    std::vector<gr_complex> d_csi;
};

} // namespace ieee802_11
} // namespace gr

#endif /* INCLUDED_IEEE802_11_PILOT_CSI_DATASET_IMPL_H */
