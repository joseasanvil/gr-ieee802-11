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

#include "pilot_csi_dataset_impl.h"

#include <gnuradio/io_signature.h>

#include <boost/bind/bind.hpp>

#include <chrono>
#include <complex>
#include <stdexcept>

namespace gr {
namespace ieee802_11 {

pilot_csi_dataset::sptr pilot_csi_dataset::make(const std::string& filename, bool append)
{
    return gnuradio::get_initial_sptr(new pilot_csi_dataset_impl(filename, append));
}

pilot_csi_dataset_impl::pilot_csi_dataset_impl(const std::string& filename, bool append)
    : block("pilot_csi_dataset",
            gr::io_signature::make(0, 0, 0),
            gr::io_signature::make(0, 0, 0)),
      d_append(append),
      d_header_written(append),
      d_has_pilots(false),
      d_has_csi(false),
      d_seq(0)
{
    std::ios::openmode mode = std::ios::out;
    if (append) {
        mode |= std::ios::app;
    } else {
        mode |= std::ios::trunc;
    }

    d_file.open(filename, mode);
    if (!d_file.is_open()) {
        throw std::runtime_error("pilot_csi_dataset: failed to open file");
    }

    message_port_register_in(pmt::mp("pilots"));
    message_port_register_in(pmt::mp("csi"));

    set_msg_handler(pmt::mp("pilots"),
                    boost::bind(&pilot_csi_dataset_impl::handle_pilots,
                                this,
                                boost::placeholders::_1));
    set_msg_handler(pmt::mp("csi"),
                    boost::bind(&pilot_csi_dataset_impl::handle_csi,
                                this,
                                boost::placeholders::_1));
}

pilot_csi_dataset_impl::~pilot_csi_dataset_impl() { d_file.close(); }

void pilot_csi_dataset_impl::handle_pilots(pmt::pmt_t msg)
{
    if (!pmt::is_pair(msg)) {
        throw std::invalid_argument("pilot_csi_dataset expects PDU pair for pilots");
    }

    pmt::pmt_t payload = pmt::cdr(msg);
    if (!pmt::is_c32vector(payload)) {
        throw std::invalid_argument("pilot_csi_dataset expects c32vector pilots");
    }

    std::lock_guard<std::mutex> lock(d_mutex);
    d_pilots = pmt::c32vector_elements(payload);
    d_has_pilots = true;
    maybe_write_locked();
}

void pilot_csi_dataset_impl::handle_csi(pmt::pmt_t msg)
{
    if (!pmt::is_pair(msg)) {
        throw std::invalid_argument("pilot_csi_dataset expects PDU pair for csi");
    }

    pmt::pmt_t payload = pmt::cdr(msg);
    if (!pmt::is_c32vector(payload)) {
        throw std::invalid_argument("pilot_csi_dataset expects c32vector csi");
    }

    std::lock_guard<std::mutex> lock(d_mutex);
    d_csi = pmt::c32vector_elements(payload);
    d_has_csi = true;
    maybe_write_locked();
}

void pilot_csi_dataset_impl::write_header_locked()
{
    if (d_header_written) {
        return;
    }

    d_file << "seq,timestamp_ns,pilots_len,csi_len";
    for (size_t i = 0; i < d_pilots.size(); i++) {
        d_file << ",p" << i << "_re"
               << ",p" << i << "_im";
    }
    for (size_t i = 0; i < d_csi.size(); i++) {
        d_file << ",c" << i << "_re"
               << ",c" << i << "_im";
    }
    for (size_t i = 0; i < d_csi.size(); i++) {
        d_file << ",h" << i << "_mag"
               << ",h" << i << "_phase";
    }
    d_file << "\n";
    d_header_written = true;
}

void pilot_csi_dataset_impl::maybe_write_locked()
{
    if (!d_has_pilots || !d_has_csi) {
        return;
    }

    write_header_locked();

    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto timestamp_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();

    d_file << d_seq++ << "," << timestamp_ns << "," << d_pilots.size() << ","
           << d_csi.size();
    for (const auto& sample : d_pilots) {
        d_file << "," << std::real(sample) << "," << std::imag(sample);
    }
    for (const auto& sample : d_csi) {
        d_file << "," << std::real(sample) << "," << std::imag(sample);
    }
    for (const auto& sample : d_csi) {
        d_file << "," << std::abs(sample) << "," << std::arg(sample);
    }
    d_file << "\n";
    d_file.flush();

    d_has_pilots = false;
    d_has_csi = false;
}

} // namespace ieee802_11
} // namespace gr
