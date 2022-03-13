#ifndef BUILDINFO_H
#define BUILDINFO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Information about the firmware build
 *
 * This structure holds some metadata about this particular build of the firmware, including
 * the associated git revision and build type.
 */
struct BuildInfo {
    /// Branch from which the firmware is built
    const char * const gitBranch;
    /// Commit hash the firmware was built from
    const char * const gitHash;

    /// Architecture for which the firmware is built
    const char * const arch;
    /// Platform for which the firmware is built
    const char * const platform;

    /// Build type (specifies optimizations, etc.)
    const char * const buildType;
    /// User that built the firmware
    const char * const buildUser;
    /// Hostname of the build machine
    const char * const buildHost;
    /// Date/time at which the build took place
    const char * const buildDate;
};

/**
 * Global build info structure, containing information on the configuration of the firmware.
 */
extern const struct BuildInfo gBuildInfo;

#ifdef __cplusplus
}
#endif

#endif
