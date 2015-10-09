// baltzo_localdatetime.cpp                                           -*-C++-*-
#include <baltzo_localdatetime.h>

#include <bsls_ident.h>
BSLS_IDENT_RCSID(baltzo_localdatetime_cpp,"$Id$ $CSID$")

#include <bslim_printer.h>

#include <bsl_ostream.h>

namespace BloombergLP {

                            // -------------------
                            // class LocalDatetime
                            // -------------------

// ACCESSORS
                        // Aspects

bsl::ostream& baltzo::LocalDatetime::print(bsl::ostream& stream,
                                           int           level,
                                           int           spacesPerLevel) const
{
    if (stream.bad()) {
        return stream;                                                // RETURN
    }

    bslim::Printer printer(&stream, level, spacesPerLevel);
    printer.start();
    printer.printAttribute("datetimeTz", d_datetimeTz);
    printer.printAttribute("timeZoneId", d_timeZoneId.c_str());
    printer.end();

    return stream;
}

// FREE OPERATORS
bsl::ostream& baltzo::operator<<(bsl::ostream&        stream,
                                 const LocalDatetime& localDatetime)
{
    if (stream.bad()) {
        return stream;                                                // RETURN
    }

    bslim::Printer printer(&stream, 0, -1);
    printer.start();
    printer.printValue(localDatetime.datetimeTz());
    printer.printValue(localDatetime.timeZoneId().c_str());
    printer.end();

    return stream;
}

}  // close enterprise namespace

// ----------------------------------------------------------------------------
// Copyright 2015 Bloomberg Finance L.P.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------- END-OF-FILE ----------------------------------