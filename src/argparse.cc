/* Argparse implementation details.
 *
 * This file is part of libostd. See COPYING.md for futher information.
 */

#include "ostd/argparse.hh"

namespace ostd {

/* place the vtables in here */

arg_error::~arg_error() {}

arg_description::~arg_description() {}
arg_argument::~arg_argument() {}
arg_optional::~arg_optional() {}
arg_positional::~arg_positional() {}

arg_mutually_exclusive_group::~arg_mutually_exclusive_group() {}
arg_group::~arg_group() {}

} /* namespace ostd */
