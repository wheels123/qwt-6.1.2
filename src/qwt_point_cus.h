/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

/*! \file */
#ifndef QWT_POINT_CUS_H
#define QWT_POINT_CUS_H 1

#include "qwt_global.h"
#include <qpoint.h>
#ifndef QT_NO_DEBUG_STREAM
#include <qdebug.h>
#endif

/*!
  \brief QwtPointCus class defines a 3D point in double coordinates
*/

class QWT_EXPORT QwtPointCus
{
public:
    enum PointStyle
    {
        NoType = -1,

        Normal,

        Label,

        Dest,

        PathPoint,

        Connect
    };
public:
    QwtPointCus();
    QwtPointCus( PointStyle type, int id, double x, double y, double phi );

    bool isNull()    const;
    inline int netSize()const;
    PointStyle type() const;
    inline int id() const;
    inline double x() const;
    inline double y() const;
    inline double phi() const;
    inline int net(int n)const;
    inline int netId()const;
    PointStyle &rtype();
    inline int &rid();
    inline double &rx();
    inline double &ry();
    inline double &rphi();
    inline int &rnet(int n);
    inline int &rnetId();
    inline void setType( PointStyle type );
    inline void setId( int id );
    inline void setX( double x );
    inline void setY( double y );
    inline void setPhi( double yphi);
    inline void setNet( int n,int net );
    inline void insertNet( int net );
    inline void deleteNet( int net );
    QPointF toPoint() const;

    bool operator==( const QwtPointCus & ) const;
    bool operator!=( const QwtPointCus & ) const;
    void operator=( const QwtPointCus & );
private:
    enum {SIZE = 10};
    PointStyle d_type;
    int d_id;
    double d_x;
    double d_y;
    double d_phi;
    int d_netId;
    int d_net[SIZE];
};

Q_DECLARE_TYPEINFO(QwtPointCus, Q_MOVABLE_TYPE);

#ifndef QT_NO_DEBUG_STREAM
QWT_EXPORT QDebug operator<<( QDebug, const QwtPointCus & );
#endif


inline QwtPointCus::QwtPointCus( PointStyle type, int id, double x, double y, double phi )
{
    d_type=type;
    d_id=id;
    d_x=x;
    d_y=y;
    d_phi=phi;
    d_netId=0;
    for(int i=0;i<SIZE;i++)
    {
        d_net[i]=0;
    }
}

/*!
    Constructs a null point.
    \sa isNull()
*/
inline QwtPointCus::QwtPointCus():
    d_type(Normal),
    d_id(0),
    d_x( 0.0 ),
    d_y( 0.0 ),
    d_phi( 0.0 ),
    d_netId(0)
{
}


/*!
    \return True if the point is null; otherwise returns false.

    A point is considered to be null if x, y and z-coordinates
    are equal to zero.
*/
inline bool QwtPointCus::isNull() const
{
    return d_type==NoType;
}

//! \return The type of the point.
inline QwtPointCus::PointStyle QwtPointCus::type() const
{
    return d_type;
}

//! \return The id the point.
inline int QwtPointCus::id() const
{
    return d_id;
}

//! \return The x-coordinate of the point.
inline double QwtPointCus::x() const
{
    return d_x;
}

//! \return The y-coordinate of the point.
inline double QwtPointCus::y() const
{
    return d_y;
}

//! \return The z-coordinate of the point.
inline double QwtPointCus::phi() const
{
    return d_phi;
}

//! \return The z-coordinate of the point.
inline int QwtPointCus::netSize() const
{
    return SIZE;
}

//! \return The z-coordinate of the point.
inline int QwtPointCus::net(int n) const
{
    if(n<SIZE)
    return d_net[n];
}

//! \return The z-coordinate of the point.
inline int QwtPointCus::netId() const
{
    return d_netId;
}

//! \return A reference to the type of the point.
inline QwtPointCus::PointStyle &QwtPointCus::rtype()
{
    return d_type;
}

//! \return A reference to the id of the point.
inline int &QwtPointCus::rid()
{
    return d_id;
}

//! \return A reference to the x-coordinate of the point.
inline double &QwtPointCus::rx()
{
    return d_x;
}

//! \return A reference to the y-coordinate of the point.
inline double &QwtPointCus::ry()
{
    return d_y;
}

//! \return A reference to the phi of the point.
inline double &QwtPointCus::rphi()
{
    return d_phi;
}

//! \return The z-coordinate of the point.
inline int &QwtPointCus::rnet(int n)
{
    if(n>SIZE-1)
    {
        n=SIZE-1;
    }
    return d_net[n];
}

//! \return A reference to the phi of the point.
inline int &QwtPointCus::rnetId()
{
    return d_netId;
}


//! Sets the type of the point to the value specified by x.
inline void QwtPointCus::setType( QwtPointCus::PointStyle type )
{
    d_type = type;
}

//! Sets the id of the point to the value specified by y.
inline void QwtPointCus::setId( int id )
{
    d_id = id;
}

//! Sets the x-coordinate of the point to the value specified by x.
inline void QwtPointCus::setX( double x )
{
    d_x = x;
}

//! Sets the y-coordinate of the point to the value specified by y.
inline void QwtPointCus::setY( double y )
{
    d_y = y;
}

//! Sets the z-coordinate of the point to the value specified by z.
inline void QwtPointCus::setPhi( double phi )
{
    d_phi = phi;
}

//! Sets the z-coordinate of the point to the value specified by z.
inline void QwtPointCus::setNet( int n,int net )
{
    if(n<SIZE)
    {
        d_net[n] = net;
    }
}

//! Sets the z-coordinate of the point to the value specified by z.
inline void QwtPointCus::insertNet( int net )
{
    /*
    for(int i=0;i<SIZE;i++)
    {
        if(d_net[i]==net)
        {
            return;
        }
    }

    d_net[d_netId] = net;
    d_netId++;
    if(d_netId>SIZE-1) d_netId=0;

    */
    for(int i=0;i<SIZE;i++)
    {
        if(d_net[i]==net)
        {
            return;
        }
    }
    int in_id=0;
    for(int i=0;i<SIZE;i++)
    {
        if(d_net[i]<=0)
        {
            in_id = i;
            break;
        }
    }
    d_net[in_id]=net;
}


//! Sets the z-coordinate of the point to the value specified by z.
inline void QwtPointCus::deleteNet( int net )
{
    /*
    int i,j,delId=-1;
    for( i=0;i<SIZE;)
    {
        if(d_net[i]==net)
        {
            d_net[i]=0;
            delId=i;
            for( j=i;j<SIZE-1;j++)
            {
                d_net[j]=d_net[j+1];
            }
            d_net[SIZE-1]=0;
            if(delId!=-1&&delId<d_netId&&d_netId>0)
            {
                d_netId-=1;
            }
        }
        else
        {
            i++;
        }
    }
*/
    for( int i=0;i<SIZE;i++)
    {
        if(d_net[i]==net)
        {
            d_net[i]=0;
        }
    }
}

//! \return True, if this point and other are equal; otherwise returns false.
inline bool QwtPointCus::operator==( const QwtPointCus &other ) const
{
    return ( d_type == other.d_type ) && ( d_id == other.d_id );
}

//! \return True if this rect and other are different; otherwise returns false.
inline bool QwtPointCus::operator!=( const QwtPointCus &other ) const
{
    return !operator==( other );
}

//! \return True if this rect and other are different; otherwise returns false.
inline void QwtPointCus::operator=( const QwtPointCus &other )
{
    d_type=other.d_type;
    d_id=other.d_id;
    d_x=other.d_x;
    d_y=other.d_y;
    d_phi=other.d_phi;
    d_netId=other.d_netId;

    for(int i=0;i<SIZE;i++)
    {
       d_net[i]=other.d_net[i];
    }
}

#endif
