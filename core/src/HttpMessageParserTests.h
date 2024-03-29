// Amanuensis - Web Traffic Inspector
//
// Copyright (C) 2017 Benjamin Bader
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <QObject>

class HttpMessageParserTests : public QObject
{
    Q_OBJECT

public:
    HttpMessageParserTests();

private Q_SLOTS:
    void simpleGet();
    void fixedLengthSimplePost();
    void chunkedSimplePost();

    void simpleOkResponse();
    void simpleForbiddenResponse();

    void connectFromEdge();

    void pauses_on_phase_transitions();

    void zero_prefixed_chunk_lengths();
};
