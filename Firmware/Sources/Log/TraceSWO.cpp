#include "TraceSWO.h"

#include <vendor/sam.h>

using namespace Log;

/**
 * @brief Initialize Trace SWO output
 *
 * This sets up the SWO port; some debuggers do this during attachment but this ensures the
 * interface is available.
 */
void TraceSWO::Init() {

}

/**
 * @brief Output character to Trace SWO
 *
 * Dumps the specified character to the SWO output.
 *
 * @param ch Character to output
 */
void TraceSWO::PutChar(const char ch) {

}
