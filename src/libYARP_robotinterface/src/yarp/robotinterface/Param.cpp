/*
 * Copyright (C) 2006-2020 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * BSD-3-Clause license. See the accompanying LICENSE file for details.
 */

#include "Param.h"

#include <yarp/os/LogStream.h>

#include <yarp/os/Property.h>

#include <string>


std::ostream& std::operator<<(std::ostream &oss, const yarp::robotinterface::Param &t)
{
    oss << "(\"" << t.name() << "\"" << (t.isGroup() ? " [group]" : "") << " = \"" << t.value() << "\")";
    return oss;
}


class yarp::robotinterface::Param::Private
{
public:
    Private(Param * /*parent*/) :
            isGroup(false)
    {
    }

    std::string name;
    std::string value;
    bool isGroup;
};

yarp::os::LogStream operator<<(yarp::os::LogStream dbg, const yarp::robotinterface::Param &t)
{
    std::ostringstream oss;
    oss << t;
    dbg << oss.str();
    return dbg;
}

yarp::robotinterface::Param::Param(bool isGroup) :
    mPriv(new Private(this))
{
    mPriv->isGroup = isGroup;
}

yarp::robotinterface::Param::Param(const std::string &name, const std::string &value, bool isGroup) :
    mPriv(new Private(this))
{
    mPriv->name = name;
    mPriv->value = value;
    mPriv->isGroup = isGroup;
}

yarp::robotinterface::Param::Param(const ::yarp::robotinterface::Param& other) :
    mPriv(new Private(this))
{
    mPriv->name = other.mPriv->name;
    mPriv->value = other.mPriv->value;
    mPriv->isGroup = other.mPriv->isGroup;
}

yarp::robotinterface::Param& yarp::robotinterface::Param::operator=(const yarp::robotinterface::Param& other)
{
    if (&other != this) {
        mPriv->name = other.mPriv->name;
        mPriv->value = other.mPriv->value;
        mPriv->isGroup = other.mPriv->isGroup;
    }

    return *this;
}

yarp::robotinterface::Param::~Param()
{
    delete mPriv;
}

std::string& yarp::robotinterface::Param::name()
{
    return mPriv->name;
}

std::string& yarp::robotinterface::Param::value()
{
    return mPriv->value;
}

const std::string& yarp::robotinterface::Param::name() const
{
    return mPriv->name;
}

const std::string& yarp::robotinterface::Param::value() const
{
    return mPriv->value;
}

bool yarp::robotinterface::Param::isGroup() const
{
    return mPriv->isGroup;
}

yarp::os::Property yarp::robotinterface::Param::toProperty() const
{
    yarp::os::Property p;
    std::string s = "(" + mPriv->name + " " + mPriv->value + ")";
    p.fromString(s);
    return p;
}
