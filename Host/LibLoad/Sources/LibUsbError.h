#ifndef LIBUSBERROR_H
#define LIBUSBERROR_H

#include <stdexcept>
#include <string>
#include <string_view>

#include <libusb.h>
#include <fmt/core.h>

namespace LibLoad::Internal {
/**
 * @brief LibUSB Exception type
 *
 * This exception wraps up an error returned by libusb.
 */
class LibUsbError: public std::runtime_error {
    public:
        LibUsbError(int error) : LibUsbError(error, "unspecified") {};

        /**
         * @brief Create a new LibUSB error with a given error code and string
         *
         * @param error A LibUSB error code (from LIBUSB_ERROR)
         * @param what Descriptive message string
         */
        LibUsbError(int error, const std::string_view &what) : std::runtime_error("libusb"),
            whatStr(fmt::format("libusb failure ({}): {}", what, libusb_error_name(error))) {}

        /// Get the useful descriptive string
        const char* what() const noexcept override {
            return this->whatStr.c_str();
        }

    private:
        /// Formatted error string
        std::string whatStr;
};
}

#endif
