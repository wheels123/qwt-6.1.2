/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include "qwt_plot_curve_cus.h"
#include "qwt_point_data.h"
#include "qwt_math.h"
#include "qwt_clipper.h"
#include "qwt_painter.h"
#include "qwt_scale_map.h"
#include "qwt_plot.h"
#include "qwt_curve_fitter.h"
#include "qwt_symbol.h"
#include "qwt_point_mapper.h"
#include <qpainter.h>
#include <qpixmap.h>
#include <qalgorithms.h>
#include <qmath.h>

static void qwtUpdateLegendIconSize( QwtPlotCurveCus *curve )
{
    if ( curve->symbol() && 
        curve->testLegendAttribute( QwtPlotCurveCus::LegendShowSymbol ) )
    {
        QSize sz = curve->symbol()->boundingRect().size();
        sz += QSize( 2, 2 ); // margin

        if ( curve->testLegendAttribute( QwtPlotCurveCus::LegendShowLine ) )
        {
            // Avoid, that the line is completely covered by the symbol

            int w = qCeil( 1.5 * sz.width() );
            if ( w % 2 )
                w++;

            sz.setWidth( qMax( 8, w ) );
        }

        curve->setLegendIconSize( sz );
    }
}

static int qwtVerifyRange( int size, int &i1, int &i2 )
{
    if ( size < 1 )
        return 0;

    i1 = qBound( 0, i1, size - 1 );
    i2 = qBound( 0, i2, size - 1 );

    if ( i1 > i2 )
        qSwap( i1, i2 );

    return ( i2 - i1 + 1 );
}

class QwtPlotCurveCus::PrivateData
{
public:
    PrivateData():
        style( QwtPlotCurveCus::Lines ),
        baseline( 0.0 ),
        symbol( NULL ),
        attributes( 0 ),
        paintAttributes( 
            QwtPlotCurveCus::ClipPolygons | QwtPlotCurveCus::FilterPoints ),
        legendAttributes( 0 )
    {
        pen = QPen( Qt::black );
        curveFitter = new QwtSplineCurveFitter;
    }

    ~PrivateData()
    {
        delete symbol;
        delete curveFitter;
    }

    QwtPlotCurveCus::CurveStyle style;
    double baseline;

    const QwtSymbol *symbol;
    QwtCurveFitter *curveFitter;

    QPen pen;
    QBrush brush;

    QwtPlotCurveCus::CurveAttributes attributes;
    QwtPlotCurveCus::PaintAttributes paintAttributes;

    QwtPlotCurveCus::LegendAttributes legendAttributes;
};

/*!
  Constructor
  \param title Title of the curve
*/
QwtPlotCurveCus::QwtPlotCurveCus( const QwtText &title ):
    QwtPlotSeriesItem( title )
{
    init();
}

/*!
  Constructor
  \param title Title of the curve
*/
QwtPlotCurveCus::QwtPlotCurveCus( const QString &title ):
    QwtPlotSeriesItem( QwtText( title ) )
{
    init();
}

//! Destructor
QwtPlotCurveCus::~QwtPlotCurveCus()
{
    delete d_data;
}

//! Initialize internal members
void QwtPlotCurveCus::init()
{
    setItemAttribute( QwtPlotItem::Legend );
    setItemAttribute( QwtPlotItem::AutoScale );

    d_data = new PrivateData;
    setData( new QwtPointCusSeriesData() );

    setZ( 20.0 );
}

//! \return QwtPlotItem::Rtti_PlotCurve
int QwtPlotCurveCus::rtti() const
{
    return QwtPlotItem::Rtti_PlotCurveCus;
}

/*!
  Specify an attribute how to draw the curve

  \param attribute Paint attribute
  \param on On/Off
  \sa testPaintAttribute()
*/
void QwtPlotCurveCus::setPaintAttribute( PaintAttribute attribute, bool on )
{
    if ( on )
        d_data->paintAttributes |= attribute;
    else
        d_data->paintAttributes &= ~attribute;
}

/*!
    \return True, when attribute is enabled
    \sa setPaintAttribute()
*/
bool QwtPlotCurveCus::testPaintAttribute( PaintAttribute attribute ) const
{
    return ( d_data->paintAttributes & attribute );
}

/*!
  Specify an attribute how to draw the legend icon

  \param attribute Attribute
  \param on On/Off
  /sa testLegendAttribute(). legendIcon()
*/
void QwtPlotCurveCus::setLegendAttribute( LegendAttribute attribute, bool on )
{
    if ( on != testLegendAttribute( attribute ) )
    {
        if ( on )
            d_data->legendAttributes |= attribute;
        else
            d_data->legendAttributes &= ~attribute;

        qwtUpdateLegendIconSize( this );
        legendChanged();
    }
}

/*!
  \return True, when attribute is enabled
  \sa setLegendAttribute()
*/
bool QwtPlotCurveCus::testLegendAttribute( LegendAttribute attribute ) const
{
    return ( d_data->legendAttributes & attribute );
}

/*!
  Set the curve's drawing style

  \param style Curve style
  \sa style()
*/
void QwtPlotCurveCus::setStyle( CurveStyle style )
{
    if ( style != d_data->style )
    {
        d_data->style = style;

        legendChanged();
        itemChanged();
    }
}

/*!
  \return Style of the curve
  \sa setStyle()
*/
QwtPlotCurveCus::CurveStyle QwtPlotCurveCus::style() const
{
    return d_data->style;
}

/*!
  \brief Assign a symbol

  The curve will take the ownership of the symbol, hence the previously
  set symbol will be delete by setting a new one. If \p symbol is 
  \c NULL no symbol will be drawn.

  \param symbol Symbol
  \sa symbol()
*/
void QwtPlotCurveCus::setSymbol( QwtSymbol *symbol )
{
    if ( symbol != d_data->symbol )
    {
        delete d_data->symbol;
        d_data->symbol = symbol;

        qwtUpdateLegendIconSize( this );

        legendChanged();
        itemChanged();
    }
}

/*!
  \return Current symbol or NULL, when no symbol has been assigned
  \sa setSymbol()
*/
const QwtSymbol *QwtPlotCurveCus::symbol() const
{
    return d_data->symbol;
}

/*!
  Build and assign a pen

  In Qt5 the default pen width is 1.0 ( 0.0 in Qt4 ) what makes it
  non cosmetic ( see QPen::isCosmetic() ). This method has been introduced
  to hide this incompatibility.

  \param color Pen color
  \param width Pen width
  \param style Pen style

  \sa pen(), brush()
 */
void QwtPlotCurveCus::setPen( const QColor &color, qreal width, Qt::PenStyle style )
{
    setPen( QPen( color, width, style ) );
}

/*!
  Assign a pen

  \param pen New pen
  \sa pen(), brush()
*/
void QwtPlotCurveCus::setPen( const QPen &pen )
{
    if ( pen != d_data->pen )
    {
        d_data->pen = pen;

        legendChanged();
        itemChanged();
    }
}

/*!
  \return Pen used to draw the lines
  \sa setPen(), brush()
*/
const QPen& QwtPlotCurveCus::pen() const
{
    return d_data->pen;
}

/*!
  \brief Assign a brush.

   In case of brush.style() != QBrush::NoBrush
   and style() != QwtPlotCurveCus::Sticks
   the area between the curve and the baseline will be filled.

   In case !brush.color().isValid() the area will be filled by
   pen.color(). The fill algorithm simply connects the first and the
   last curve point to the baseline. So the curve data has to be sorted
   (ascending or descending).

  \param brush New brush
  \sa brush(), setBaseline(), baseline()
*/
void QwtPlotCurveCus::setBrush( const QBrush &brush )
{
    if ( brush != d_data->brush )
    {
        d_data->brush = brush;

        legendChanged();
        itemChanged();
    }
}

/*!
  \return Brush used to fill the area between lines and the baseline
  \sa setBrush(), setBaseline(), baseline()
*/
const QBrush& QwtPlotCurveCus::brush() const
{
    return d_data->brush;
}

/*!
  Draw an interval of the curve

  \param painter Painter
  \param xMap Maps x-values into pixel coordinates.
  \param yMap Maps y-values into pixel coordinates.
  \param canvasRect Contents rectangle of the canvas
  \param from Index of the first point to be painted
  \param to Index of the last point to be painted. If to < 0 the
         curve will be painted to its last point.

  \sa drawCurve(), drawSymbols(),
*/
void QwtPlotCurveCus::drawSeries( QPainter *painter,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRectF &canvasRect, int from, int to ) const
{
    const size_t numSamples = dataSize();

    if ( !painter || numSamples <= 0 )
        return;

    if ( to < 0 )
        to = numSamples - 1;

    if ( qwtVerifyRange( numSamples, from, to ) > 0 )
    {
        painter->save();
        painter->setPen( d_data->pen );

        /*
          Qt 4.0.0 is slow when drawing lines, but it's even
          slower when the painter has a brush. So we don't
          set the brush before we really need it.
         */

        drawCurve( painter, d_data->style, xMap, yMap, canvasRect, from, to );
        painter->restore();

        int realEnd=to;

        if ( d_data->symbol &&
            ( d_data->symbol->style() != QwtSymbol::NoSymbol ) )
        {
            painter->save();
            drawSymbols( painter, *d_data->symbol,
                xMap, yMap, canvasRect, from, realEnd );
            painter->restore();
        }
    }
}

/*!
  \brief Draw the line part (without symbols) of a curve interval.
  \param painter Painter
  \param style curve style, see QwtPlotCurveCus::CurveStyle
  \param xMap x map
  \param yMap y map
  \param canvasRect Contents rectangle of the canvas
  \param from index of the first point to be painted
  \param to index of the last point to be painted
  \sa draw(), drawDots(), drawLines(), drawSteps(), drawSticks()
*/
void QwtPlotCurveCus::drawCurve( QPainter *painter, int style,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRectF &canvasRect, int from, int to ) const
{
    switch ( style )
    {
        case Lines:
            if ( testCurveAttribute( Fitted ) )
            {
                // we always need the complete
                // curve for fitting
                from = 0;
                to = dataSize() - 1;
            }
            drawLines( painter, xMap, yMap, canvasRect, from, to );
            break;
        case Sticks:
            drawSticks( painter, xMap, yMap, canvasRect, from, to );
            break;
        case Steps:
            drawSteps( painter, xMap, yMap, canvasRect, from, to );
            break;
        case Dots:
            drawDots( painter, xMap, yMap, canvasRect, from, to );
            break;
        case NetWorkLines:
            if ( testCurveAttribute( Fitted ) )
            {
                // we always need the complete
                // curve for fitting
                from = 0;
                to = dataSize() - 1;
            }
            drawNetWorkLines( painter, xMap, yMap, canvasRect, from, to );
            break;
        case NoCurve:
        default:
            break;
    }
}

/*!
  \brief Draw lines

  If the CurveAttribute Fitted is enabled a QwtCurveFitter tries
  to interpolate/smooth the curve, before it is painted.

  \param painter Painter
  \param xMap x map
  \param yMap y map
  \param canvasRect Contents rectangle of the canvas
  \param from index of the first point to be painted
  \param to index of the last point to be painted

  \sa setCurveAttribute(), setCurveFitter(), draw(),
      drawLines(), drawDots(), drawSteps(), drawSticks()
*/
void QwtPlotCurveCus::drawLines( QPainter *painter,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRectF &canvasRect, int from, int to ) const
{
    if ( from > to )
        return;

    const bool doAlign = QwtPainter::roundingAlignment( painter );
    const bool doFit = ( d_data->attributes & Fitted ) && d_data->curveFitter;
    const bool doFill = ( d_data->brush.style() != Qt::NoBrush )
            && ( d_data->brush.color().alpha() > 0 );

    QRectF clipRect;
    if ( d_data->paintAttributes & ClipPolygons )
    {
        qreal pw = qMax( qreal( 1.0 ), painter->pen().widthF());
        clipRect = canvasRect.adjusted(-pw, -pw, pw, pw);
    }

    bool doIntegers = false;

#if QT_VERSION < 0x040800

    // For Qt <= 4.7 the raster paint engine is significantly faster
    // for rendering QPolygon than for QPolygonF. So let's
    // see if we can use it.

    if ( painter->paintEngine()->type() == QPaintEngine::Raster )
    {
        // In case of filling or fitting performance doesn't count
        // because both operations are much more expensive
        // then drawing the polyline itself

        if ( !doFit && !doFill )
            doIntegers = true; 
    }
#endif

    const bool noDuplicates = d_data->paintAttributes & FilterPoints;

    QwtPointMapper mapper;
    mapper.setFlag( QwtPointMapper::RoundPoints, doAlign );
    mapper.setFlag( QwtPointMapper::WeedOutPoints, noDuplicates );
    mapper.setBoundingRect( canvasRect );

    const QwtSeriesData<QwtPointCus> *seriesCus = data();
    QVector<QPointF> p(seriesCus->size());
    p.resize(seriesCus->size());
    p.clear();
    for ( int i = from; i <= to; i++ )
    {
       QPointF sample;
       sample.rx() = seriesCus->sample( i ).x();
       sample.ry() = seriesCus->sample( i ).y();
       p.append(sample);
    }

    QwtSeriesData<QPointF> *series = new QwtPointSeriesData(p);

    if ( doIntegers )
    {
        QPolygon polyline = mapper.toPolygon( 
            xMap, yMap, series, from, to );

        if ( d_data->paintAttributes & ClipPolygons )
        {
            polyline = QwtClipper::clipPolygon( 
                clipRect.toAlignedRect(), polyline, false );
        }

        QwtPainter::drawPolyline( painter, polyline );
    }
    else
    {
        QPolygonF polyline = mapper.toPolygonF( xMap, yMap, series, from, to );

        if ( doFit )
            polyline = d_data->curveFitter->fitCurve( polyline );

        if ( doFill )
        {
            if ( painter->pen().style() != Qt::NoPen )
            {
                // here we are wasting memory for the filled copy,
                // do polygon clipping twice etc .. TODO

                QPolygonF filled = polyline;
                fillCurve( painter, xMap, yMap, canvasRect, filled );
                filled.clear();

                if ( d_data->paintAttributes & ClipPolygons )
                {
                    polyline = QwtClipper::clipPolygonF( 
                        clipRect, polyline, false );
                }

                QwtPainter::drawPolyline( painter, polyline );
            }
            else
            {
                fillCurve( painter, xMap, yMap, canvasRect, polyline );
            }
        }
        else
        {
            if ( d_data->paintAttributes & ClipPolygons )
            {
                polyline = QwtClipper::clipPolygonF(
                    clipRect, polyline, false );
            }

            QwtPainter::drawPolyline( painter, polyline );
        }
    }

}

/*!
  Draw sticks

  \param painter Painter
  \param xMap x map
  \param yMap y map
  \param canvasRect Contents rectangle of the canvas
  \param from index of the first point to be painted
  \param to index of the last point to be painted

  \sa draw(), drawCurve(), drawDots(), drawLines(), drawSteps()
*/
void QwtPlotCurveCus::drawSticks( QPainter *painter,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRectF &, int from, int to ) const
{
    painter->save();
    painter->setRenderHint( QPainter::Antialiasing, false );

    const bool doAlign = QwtPainter::roundingAlignment( painter );

    double x0 = xMap.transform( d_data->baseline );
    double y0 = yMap.transform( d_data->baseline );
    if ( doAlign )
    {
        x0 = qRound( x0 );
        y0 = qRound( y0 );
    }
/*
    const Qt::Orientation o = orientation();

    const QwtSeriesData<QPointF> *series = data();

    for ( int i = from; i <= to; i++ )
    {
        const QPointF sample = series->sample( i );
        double xi = xMap.transform( sample.x() );
        double yi = yMap.transform( sample.y() );
        if ( doAlign )
        {
            xi = qRound( xi );
            yi = qRound( yi );
        }

        if ( o == Qt::Horizontal )
            QwtPainter::drawLine( painter, x0, yi, xi, yi );
        else
            QwtPainter::drawLine( painter, xi, y0, xi, yi );
    }

    painter->restore();*/
}

/*!
  Draw dots

  \param painter Painter
  \param xMap x map
  \param yMap y map
  \param canvasRect Contents rectangle of the canvas
  \param from index of the first point to be painted
  \param to index of the last point to be painted

  \sa draw(), drawCurve(), drawSticks(), drawLines(), drawSteps()
*/
void QwtPlotCurveCus::drawDots( QPainter *painter,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRectF &canvasRect, int from, int to ) const
{
    const QColor color = painter->pen().color();

    if ( painter->pen().style() == Qt::NoPen || color.alpha() == 0 )
    {
        return;
    }

    const bool doFill = ( d_data->brush.style() != Qt::NoBrush )
            && ( d_data->brush.color().alpha() > 0 );
    const bool doAlign = QwtPainter::roundingAlignment( painter );

    QwtPointMapper mapper;
    mapper.setBoundingRect( canvasRect );
    mapper.setFlag( QwtPointMapper::RoundPoints, doAlign );

    if ( d_data->paintAttributes & FilterPoints )
    {
        if ( ( color.alpha() == 255 )
            && !( painter->renderHints() & QPainter::Antialiasing ) )
        {
            mapper.setFlag( QwtPointMapper::WeedOutPoints, true );
        }
    }
/*
    if ( doFill )
    {
        mapper.setFlag( QwtPointMapper::WeedOutPoints, false );

        QPolygonF points = mapper.toPointsF( 
            xMap, yMap, data(), from, to );

        QwtPainter::drawPoints( painter, points );
        fillCurve( painter, xMap, yMap, canvasRect, points );
    }
    else if ( d_data->paintAttributes & ImageBuffer )
    {
        const QImage image = mapper.toImage( xMap, yMap,
            data(), from, to, d_data->pen, 
            painter->testRenderHint( QPainter::Antialiasing ),
            renderThreadCount() );

        painter->drawImage( canvasRect.toAlignedRect(), image );
    }
    else if ( d_data->paintAttributes & MinimizeMemory )
    {
        const QwtSeriesData<QPointF> *series = data();

        for ( int i = from; i <= to; i++ )
        {
            const QPointF sample = series->sample( i );

            double xi = xMap.transform( sample.x() );
            double yi = yMap.transform( sample.y() );

            if ( doAlign )
            {
                xi = qRound( xi );
                yi = qRound( yi );
            }

            QwtPainter::drawPoint( painter, QPointF( xi, yi ) );
        }
    }
    else
    {
        if ( doAlign )
        {
            const QPolygon points = mapper.toPoints(
                xMap, yMap, data(), from, to ); 

            QwtPainter::drawPoints( painter, points );
        }
        else
        {
            const QPolygonF points = mapper.toPointsF( 
                xMap, yMap, data(), from, to );

            QwtPainter::drawPoints( painter, points );
        }
    }*/
}

/*!
  Draw step function

  The direction of the steps depends on Inverted attribute.

  \param painter Painter
  \param xMap x map
  \param yMap y map
  \param canvasRect Contents rectangle of the canvas
  \param from index of the first point to be painted
  \param to index of the last point to be painted

  \sa CurveAttribute, setCurveAttribute(),
      draw(), drawCurve(), drawDots(), drawLines(), drawSticks()
*/
void QwtPlotCurveCus::drawSteps( QPainter *painter,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRectF &canvasRect, int from, int to ) const
{
    const bool doAlign = QwtPainter::roundingAlignment( painter );

    QPolygonF polygon( 2 * ( to - from ) + 1 );
    QPointF *points = polygon.data();

    bool inverted = orientation() == Qt::Vertical;
    if ( d_data->attributes & Inverted )
        inverted = !inverted;

    const QwtSeriesData<QwtPointCus> *seriesCus = data();
    QVector<QPointF> p(seriesCus->size());
    p.resize(seriesCus->size());
    p.clear();
    for ( int i = from; i <= to; i++ )
    {
       QPointF sample;
       sample.rx() = seriesCus->sample( i ).x();
       sample.ry() = seriesCus->sample( i ).y();
       p.append(sample);
    }

    QwtSeriesData<QPointF> *series = new QwtPointSeriesData(p);

    //const QwtSeriesData<QPointF> *series = data();
    int i, ip;
    for ( i = from, ip = 0; i <= to; i++, ip += 2 )
    {
        const QPointF sample = series->sample( i );
        double xi = xMap.transform( sample.x() );
        double yi = yMap.transform( sample.y() );
        if ( doAlign )
        {
            xi = qRound( xi );
            yi = qRound( yi );
        }

        if ( ip > 0 )
        {
            const QPointF &p0 = points[ip - 2];
            QPointF &p = points[ip - 1];

            if ( inverted )
            {
                p.rx() = p0.x();
                p.ry() = yi;
            }
            else
            {
                p.rx() = xi;
                p.ry() = p0.y();
            }
        }

        points[ip].rx() = xi;
        points[ip].ry() = yi;
    }

    if ( d_data->paintAttributes & ClipPolygons )
    {
        const QPolygonF clipped = QwtClipper::clipPolygonF( 
            canvasRect, polygon, false );

        QwtPainter::drawPolyline( painter, clipped );
    }
    else
    {
        QwtPainter::drawPolyline( painter, polygon );
    }

    if ( d_data->brush.style() != Qt::NoBrush )
        fillCurve( painter, xMap, yMap, canvasRect, polygon );
}

/*!
  Draw network lines function

  The direction of the steps depends on Inverted attribute.

  \param painter Painter
  \param xMap x map
  \param yMap y map
  \param canvasRect Contents rectangle of the canvas
  \param from index of the first point to be painted
  \param to index of the last point to be painted

  \sa CurveAttribute, setCurveAttribute(),
      draw(), drawCurve(), drawDots(), drawLines(), drawSticks()
*/
void QwtPlotCurveCus::drawNetWorkLines( QPainter *painter,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRectF &canvasRect, int from, int to ) const
{

    int pointNum=to-from+1;

    if(pointNum<1) return;
    const QColor color = painter->pen().color();
    painter->pen().color()=Qt::red;
    if ( painter->pen().style() == Qt::NoPen || color.alpha() == 0 )
    {
        return;
    }

    const bool doFill = ( d_data->brush.style() != Qt::NoBrush )
            && ( d_data->brush.color().alpha() > 0 );
    const bool doAlign = QwtPainter::roundingAlignment( painter );

    QwtPointMapper mapper;
    mapper.setBoundingRect( canvasRect );
    mapper.setFlag( QwtPointMapper::RoundPoints, doAlign );

    if ( d_data->paintAttributes & FilterPoints )
    {
        if ( ( color.alpha() == 255 )
            && !( painter->renderHints() & QPainter::Antialiasing ) )
        {
            mapper.setFlag( QwtPointMapper::WeedOutPoints, true );
        }
    }

    const QwtSeriesData<QwtPointCus> *seriesCus = data();
    QVector<QPointF> p(seriesCus->size());
    p.resize(seriesCus->size());
    p.clear();
    for ( int i = from; i <= to; i++ )
    {
       QPointF sample;
       sample.rx() = seriesCus->sample( i ).x();
       sample.ry() = seriesCus->sample( i ).y();
       p.append(sample);
    }

    QwtSeriesData<QPointF> *series = new QwtPointSeriesData(p);

    mapper.setFlag( QwtPointMapper::WeedOutPoints, false );
    QPolygonF points = mapper.toPointsF(
        xMap, yMap, series, from, to );
    QwtPainter::drawPoints( painter, points );
    int i,j,k;
    int netSize;
    netSize=seriesCus->sample(0).netSize();
    /*
    for ( i = from ; i < to; i++ )
    {
        for ( j = 0 ; j < netSize; j++ )
        {
            int id=seriesCus->sample(i).net(j);
            if(id!=0)
            {
                for ( k = i+1 ; k <= to; k++ )
                {
                    if(seriesCus->sample(k).id()==id)
                    {

                        const QPointF samplei = series->sample( i );
                        double xi = xMap.transform( samplei.x() );
                        double yi = yMap.transform( samplei.y() );

                        const QPointF samplek = series->sample( k );
                        double xk = xMap.transform( samplek.x() );
                        double yk = yMap.transform( samplek.y() );

                        QwtPainter::drawLine(painter,QPointF(xi,yi),QPointF(xk,yk));
                        //QwtPainter::drawLine(painter,points.at(i),points.at(k));

                        float x1 = xi;         //lastPoint 起点

                        float y1 = yi;

                        float x2 = xk;           //endPoint 终点

                        float y2 = yk;

                        float dis = sqrt((xi-xk)*(xi-xk)+(yi-yk)*(yi-yk));
                        float l = dis/8;                 //箭头的那长度

                        float a = 0.5;                       //箭头与线段角度

                        float x3 = x2 - l * cos(atan2((y2 - y1) , (x2 - x1)) - a);

                        float y3 = y2 - l * sin(atan2((y2 - y1) , (x2 - x1)) - a);

                        float x4 = x2 - l * sin(atan2((x2 - x1) , (y2 - y1)) - a);

                        float y4 = y2 - l * cos(atan2((x2 - x1) , (y2 - y1)) - a);

                         QwtPainter::drawLine(painter,x2,y2,x3,y3);

                         QwtPainter::drawLine(painter,x2,y2,x4,y4);

                        //painter.drawLine(lastPoint,endPoint);
                    }
                }
            }
        }

    }*/
    const int PATH_ID_MAX = 100000;
    for ( i = from ; i < to; i++ )
    {
        for ( j = i+1 ; j <= to; j++ )
        {
            int idi=seriesCus->sample(i).id();
            int idj=seriesCus->sample(j).id();
            bool find_i2j=false,find_j2i=false;
            if(idi<PATH_ID_MAX)
            {
                for ( int m = 0 ; m < netSize; m++ )
                {
                    int id=seriesCus->sample(i).net(m);
                    if(id!=0&&id<100000&&id==idj)
                    {
                        find_i2j=true;
                    }
                }
            }
            if(idj<PATH_ID_MAX)
            {
                for ( int n = 0 ; n < netSize; n++ )
                {
                    int id=seriesCus->sample(j).net(n);
                    if(id!=0&&id<100000&&id==idi)
                    {
                        find_j2i=true;
                    }
                }
            }

            const QPointF samplei = series->sample( i );
            double xi = xMap.transform( samplei.x() );
            double yi = yMap.transform( samplei.y() );

            const QPointF samplej = series->sample( j );
            double xj = xMap.transform( samplej.x() );
            double yj = yMap.transform( samplej.y() );

            if(find_i2j||find_j2i)
            {
                QwtPainter::drawLine(painter,QPointF(xi,yi),QPointF(xj,yj));
            }
            float x1 = xi;         //lastPoint 起点
            float y1 = yi;
            float x2 = xj;           //endPoint 终点
            float y2 = yj;
            float dis = sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
            float l = dis*0.4;                 //箭头的那长度
            float a = 0.15;                       //箭头与线段角度

            double xp = xMap.transform( 0 );
            double yp = xMap.transform( 0 );
            double xq = xMap.transform( 0.4 );
            double yq = xMap.transform( 0 );

            float l_limit=sqrt((xp-xq)*(xp-xq)+(yp-yq)*(yp-yq));
            if(l>l_limit) l=l_limit;
            if(find_i2j)
            {

                float x3 = x2 - l * cos(atan2((y2 - y1) , (x2 - x1)) - a);
                float y3 = y2 - l * sin(atan2((y2 - y1) , (x2 - x1)) - a);
                float x4 = x2 - l * sin(atan2((x2 - x1) , (y2 - y1)) - a);
                float y4 = y2 - l * cos(atan2((x2 - x1) , (y2 - y1)) - a);
                float x5 = x2 - l/2 * sin(atan2((x2 - x1) , (y2 - y1)));
                float y5 = y2 - l/2 * cos(atan2((x2 - x1) , (y2 - y1)));
                QPolygon p;
                p.append(QPoint(x5,y5));
                p.append(QPoint(x3,y3));
                p.append(QPoint(x4,y4));
                painter->setBrush(QBrush(Qt::black,Qt::SolidPattern));
                QwtPainter::drawPolygon(painter,p);
                //QwtPainter::drawLine(painter,x2,y2,x3,y3);
                //QwtPainter::drawLine(painter,x2,y2,x4,y4);
                //QwtPainter::drawLine(painter,x3,y3,x4,y4);

            }
            if(find_j2i)
            {

                float x3 = x1 - l * cos(atan2((y1 - y2) , (x1 - x2)) - a);
                float y3 = y1 - l * sin(atan2((y1 - y2) , (x1 - x2)) - a);
                float x4 = x1 - l * sin(atan2((x1 - x2) , (y1 - y2)) - a);
                float y4 = y1 - l * cos(atan2((x1 - x2) , (y1 - y2)) - a);
                float x5 = x1 - l/2 * sin(atan2((x1 - x2) , (y1 - y2)) );
                float y5 = y1 - l/2 * cos(atan2((x1 - x2) , (y1 - y2)) );
                QPolygon p;
                p.append(QPoint(x5,y5));
                p.append(QPoint(x3,y3));
                p.append(QPoint(x4,y4));
                painter->setBrush(QBrush(Qt::black,Qt::SolidPattern));
                QwtPainter::drawPolygon(painter,p);
                //QwtPainter::drawLine(painter,x1,y1,x3,y3);
                //QwtPainter::drawLine(painter,x1,y1,x4,y4);
                //QwtPainter::drawLine(painter,x3,y3,x4,y4);
            }
        }


    }


}

/*!
  Specify an attribute for drawing the curve

  \param attribute Curve attribute
  \param on On/Off

  /sa testCurveAttribute(), setCurveFitter()
*/
void QwtPlotCurveCus::setCurveAttribute( CurveAttribute attribute, bool on )
{
    if ( bool( d_data->attributes & attribute ) == on )
        return;

    if ( on )
        d_data->attributes |= attribute;
    else
        d_data->attributes &= ~attribute;

    itemChanged();
}

/*!
    \return true, if attribute is enabled
    \sa setCurveAttribute()
*/
bool QwtPlotCurveCus::testCurveAttribute( CurveAttribute attribute ) const
{
    return d_data->attributes & attribute;
}

/*!
  Assign a curve fitter

  The curve fitter "smooths" the curve points, when the Fitted
  CurveAttribute is set. setCurveFitter(NULL) also disables curve fitting.

  The curve fitter operates on the translated points ( = widget coordinates)
  to be functional for logarithmic scales. Obviously this is less performant
  for fitting algorithms, that reduce the number of points.

  For situations, where curve fitting is used to improve the performance
  of painting huge series of points it might be better to execute the fitter
  on the curve points once and to cache the result in the QwtSeriesData object.

  \param curveFitter() Curve fitter
  \sa Fitted
*/
void QwtPlotCurveCus::setCurveFitter( QwtCurveFitter *curveFitter )
{
    delete d_data->curveFitter;
    d_data->curveFitter = curveFitter;

    itemChanged();
}

/*!
  Get the curve fitter. If curve fitting is disabled NULL is returned.

  \return Curve fitter
  \sa setCurveFitter(), Fitted
*/
QwtCurveFitter *QwtPlotCurveCus::curveFitter() const
{
    return d_data->curveFitter;
}

/*!
  Fill the area between the curve and the baseline with
  the curve brush

  \param painter Painter
  \param xMap x map
  \param yMap y map
  \param canvasRect Contents rectangle of the canvas
  \param polygon Polygon - will be modified !

  \sa setBrush(), setBaseline(), setStyle()
*/
void QwtPlotCurveCus::fillCurve( QPainter *painter,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRectF &canvasRect, QPolygonF &polygon ) const
{
    if ( d_data->brush.style() == Qt::NoBrush )
        return;

    closePolyline( painter, xMap, yMap, polygon );
    if ( polygon.count() <= 2 ) // a line can't be filled
        return;

    QBrush brush = d_data->brush;
    if ( !brush.color().isValid() )
        brush.setColor( d_data->pen.color() );

    if ( d_data->paintAttributes & ClipPolygons )
        polygon = QwtClipper::clipPolygonF( canvasRect, polygon, true );

    painter->save();

    painter->setPen( Qt::NoPen );
    painter->setBrush( brush );

    QwtPainter::drawPolygon( painter, polygon );

    painter->restore();
}

/*!
  \brief Complete a polygon to be a closed polygon including the 
         area between the original polygon and the baseline.

  \param painter Painter
  \param xMap X map
  \param yMap Y map
  \param polygon Polygon to be completed
*/
void QwtPlotCurveCus::closePolyline( QPainter *painter,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    QPolygonF &polygon ) const
{
    if ( polygon.size() < 2 )
        return;

    const bool doAlign = QwtPainter::roundingAlignment( painter );

    double baseline = d_data->baseline;
    
    if ( orientation() == Qt::Vertical )
    {
        if ( yMap.transformation() )
            baseline = yMap.transformation()->bounded( baseline );

        double refY = yMap.transform( baseline );
        if ( doAlign )
            refY = qRound( refY );

        polygon += QPointF( polygon.last().x(), refY );
        polygon += QPointF( polygon.first().x(), refY );
    }
    else
    {
        if ( xMap.transformation() )
            baseline = xMap.transformation()->bounded( baseline );

        double refX = xMap.transform( baseline );
        if ( doAlign )
            refX = qRound( refX );

        polygon += QPointF( refX, polygon.last().y() );
        polygon += QPointF( refX, polygon.first().y() );
    }
}

/*!
  Draw symbols

  \param painter Painter
  \param symbol Curve symbol
  \param xMap x map
  \param yMap y map
  \param canvasRect Contents rectangle of the canvas
  \param from Index of the first point to be painted
  \param to Index of the last point to be painted

  \sa setSymbol(), drawSeries(), drawCurve()
*/
void QwtPlotCurveCus::drawSymbols( QPainter *painter, const QwtSymbol &symbol,
    const QwtScaleMap &xMap, const QwtScaleMap &yMap,
    const QRectF &canvasRect, int from, int to ) const
{
    QwtPointMapper mapper;
    mapper.setFlag( QwtPointMapper::RoundPoints, 
        QwtPainter::roundingAlignment( painter ) );
    mapper.setFlag( QwtPointMapper::WeedOutPoints, 
        testPaintAttribute( QwtPlotCurveCus::FilterPoints ) );
    mapper.setBoundingRect( canvasRect );

    const QwtSeriesData<QwtPointCus> *seriesCus = data();
    QVector<QPointF> p(seriesCus->size());
    p.resize(seriesCus->size());
    p.clear();
    for ( int i = from; i <= to; i++ )
    {
       QPointF sample;
       sample.rx() = seriesCus->sample( i ).x();
       sample.ry() = seriesCus->sample( i ).y();
       p.append(sample);
    }

    QwtSeriesData<QPointF> *series = new QwtPointSeriesData(p);

    const int chunkSize = 500;

    for ( int i = from; i <= to; i += chunkSize )
    {
        const int n = qMin( chunkSize, to - i + 1 );

        const QPolygonF points = mapper.toPointsF( xMap, yMap,
            series, i, i + n - 1 );

        if ( points.size() > 0 )
            symbol.drawSymbols( painter, points );
    }
}

/*!
  \brief Set the value of the baseline

  The baseline is needed for filling the curve with a brush or
  the Sticks drawing style.

  The interpretation of the baseline depends on the orientation().
  With Qt::Horizontal, the baseline is interpreted as a horizontal line
  at y = baseline(), with Qt::Vertical, it is interpreted as a vertical
  line at x = baseline().

  The default value is 0.0.

  \param value Value of the baseline
  \sa baseline(), setBrush(), setStyle(), QwtPlotAbstractSeriesItem::orientation()
*/
void QwtPlotCurveCus::setBaseline( double value )
{
    if ( d_data->baseline != value )
    {
        d_data->baseline = value;
        itemChanged();
    }
}

/*!
  \return Value of the baseline
  \sa setBaseline()
*/
double QwtPlotCurveCus::baseline() const
{
    return d_data->baseline;
}

/*!
  Find the closest curve point for a specific position

  \param pos Position, where to look for the closest curve point
  \param dist If dist != NULL, closestPoint() returns the distance between
              the position and the closest curve point
  \return Index of the closest curve point, or -1 if none can be found
          ( f.e when the curve has no points )
  \note closestPoint() implements a dumb algorithm, that iterates
        over all points
*/
int QwtPlotCurveCus::closestPoint( const QPoint &pos, double *dist ) const
{
    const size_t numSamples = dataSize();

    if ( plot() == NULL || numSamples <= 0 )
        return -1;

    const QwtSeriesData<QwtPointCus> *series = data();

    const QwtScaleMap xMap = plot()->canvasMap( xAxis() );
    const QwtScaleMap yMap = plot()->canvasMap( yAxis() );

    int index = -1;
    double dmin = 1.0e10;

    for ( uint i = 0; i < numSamples; i++ )
    {
        const QwtPointCus sample = series->sample( i );

        const double cx = xMap.transform( sample.x() ) - pos.x();
        const double cy = yMap.transform( sample.y() ) - pos.y();

        const double f = qwtSqr( cx ) + qwtSqr( cy );
        if ( f < dmin )
        {
            index = i;
            dmin = f;
        }
    }
    if ( dist )
        *dist = qSqrt( dmin );

    return index;

}

/*!
   \return Icon representing the curve on the legend

   \param index Index of the legend entry 
                ( ignored as there is only one )
   \param size Icon size

   \sa QwtPlotItem::setLegendIconSize(), QwtPlotItem::legendData()
 */
QwtGraphic QwtPlotCurveCus::legendIcon( int index,
    const QSizeF &size ) const
{
    Q_UNUSED( index );

    if ( size.isEmpty() )
        return QwtGraphic();

    QwtGraphic graphic;
    graphic.setDefaultSize( size );
    graphic.setRenderHint( QwtGraphic::RenderPensUnscaled, true );

    QPainter painter( &graphic );
    painter.setRenderHint( QPainter::Antialiasing,
        testRenderHint( QwtPlotItem::RenderAntialiased ) );

    if ( d_data->legendAttributes == 0 ||
        d_data->legendAttributes & QwtPlotCurveCus::LegendShowBrush )
    {
        QBrush brush = d_data->brush;

        if ( brush.style() == Qt::NoBrush &&
            d_data->legendAttributes == 0 )
        {
            if ( style() != QwtPlotCurveCus::NoCurve )
            {
                brush = QBrush( pen().color() );
            }
            else if ( d_data->symbol &&
                ( d_data->symbol->style() != QwtSymbol::NoSymbol ) )
            {
                brush = QBrush( d_data->symbol->pen().color() );
            }
        }

        if ( brush.style() != Qt::NoBrush )
        {
            QRectF r( 0, 0, size.width(), size.height() );
            painter.fillRect( r, brush );
        }
    }

    if ( d_data->legendAttributes & QwtPlotCurveCus::LegendShowLine )
    {
        if ( pen() != Qt::NoPen )
        {
            QPen pn = pen();
            pn.setCapStyle( Qt::FlatCap );

            painter.setPen( pn );

            const double y = 0.5 * size.height();
            QwtPainter::drawLine( &painter, 0.0, y, size.width(), y );
        }
    }

    if ( d_data->legendAttributes & QwtPlotCurveCus::LegendShowSymbol )
    {
        if ( d_data->symbol )
        {
            QRectF r( 0, 0, size.width(), size.height() );
            d_data->symbol->drawSymbol( &painter, r );
        }
    }

    return graphic;
}

/*!
  Initialize data with an array of points.

  \param samples Vector of points
  \note QVector is implicitly shared
  \note QPolygonF is derived from QVector<QPointF>
*/
void QwtPlotCurveCus::setSamples( const QVector<QwtPointCus> &samples )
{
    setData( new QwtPointCusSeriesData( samples ) );
}

/*!
  Assign a series of points

  setSamples() is just a wrapper for setData() without any additional
  value - beside that it is easier to find for the developer.

  \param data Data
  \warning The item takes ownership of the data object, deleting
           it when its not used anymore.
*/
void QwtPlotCurveCus::setSamples( QwtSeriesData<QwtPointCus> *data )
{
    setData( data );
}

#ifndef QWT_NO_COMPAT

/*!
  \brief Initialize the data by pointing to memory blocks which 
         are not managed by QwtPlotCurveCus.

  setRawSamples is provided for efficiency. 
  It is important to keep the pointers
  during the lifetime of the underlying QwtCPointerData class.

  \param xData pointer to x data
  \param yData pointer to y data
  \param size size of x and y

  \sa QwtCPointerData
*/
void QwtPlotCurveCus::setRawSamples(
    const double *xData, const double *yData, int size )
{
    //setData( new QwtCPointerData( xData, yData, size ) );
}

/*!
  Set data by copying x- and y-values from specified memory blocks.
  Contrary to setRawSamples(), this function makes a 'deep copy' of
  the data.

  \param xData pointer to x values
  \param yData pointer to y values
  \param size size of xData and yData

  \sa QwtPointArrayData
*/
void QwtPlotCurveCus::setSamples(
    const double *xData, const double *yData, int size )
{
    //setData( new QwtPointArrayData( xData, yData, size ) );
}

/*!
  \brief Initialize data with x- and y-arrays (explicitly shared)

  \param xData x data
  \param yData y data

  \sa QwtPointArrayData
*/
void QwtPlotCurveCus::setSamples( const QVector<QwtPointCus::PointStyle> &typeData, const QVector<int> &idData ,const QVector<double> &xData, const QVector<double> &yData ,const QVector<double> &phiData)
{
    setData(new QwtPointCusArrayData(typeData,idData,xData,yData,phiData));
}


#endif // !QWT_NO_COMPAT
