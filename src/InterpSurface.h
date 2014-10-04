//
//  InterpSurface.h
//  Interp
//
//  Created by Akira on 2014/10/04.
//
//

#ifndef Interp_InterpSurface_h
#define Interp_InterpSurface_h

#include "ofMain.h"
#include "Anemometer.h"
#include "ofxDelaunay.h"
#include "TriForInterp.h"
#include "ConnectionLine.h"
#include "Contour.h"

static const int CONNECTIONLINE_SAMPLE_NUM = 10;
static const int CONTOUR_SAMPLE_NUM = 100;

static ofPoint center;
static bool sortPosClockwise(const WindAndPosition &a, const WindAndPosition &b)
{
    if (a.pos.x - center.x >= 0 && b.pos.x - center.x < 0)
        return true;
    if (a.pos.x - center.x < 0 && b.pos.x - center.x >= 0)
        return false;
    if (a.pos.x - center.x == 0 && b.pos.x - center.x == 0)
    {
        if (a.pos.y - center.y >= 0 || b.pos.y - center.y >= 0)
            return a.pos.y > b.pos.y;
        
        return b.pos.y > a.pos.y;
    }
    
    // compute the cross product of vectors (center -> a) x (center -> b)
    int det = (a.pos.x - center.x) * (b.pos.y - center.y) - (b.pos.x - center.x) * (a.pos.y - center.y);
    if (det < 0)
        return true;
    if (det > 0)
        return false;
    
    // points a and b are on the same line from the center
    // check which point is closer to the center
    int d1 = (a.pos.x - center.x) * (a.pos.x - center.x) + (a.pos.y - center.y) * (a.pos.y - center.y);
    int d2 = (b.pos.x - center.x) * (b.pos.x - center.x) + (b.pos.y - center.y) * (b.pos.y - center.y);
    return d1 > d2;
}

class InterpSurface
{
public:
    
    void setup()
    {
        windSpeed = 40;
        
        loadAnemometer();
        triabgulateAnemometers();
        getEachTriangles();
        buildConnectionLines();
        buildContour();
    }
    
    void update()
    {
        
    }
    
    void draw()
    {
        for (auto c : contours)
        {
            c.draw();
        }
        
        ofPushStyle();
        ofNoFill();
        ofSetColor(ofColor::yellow, 100);
        for (auto c : connectionLines)
        {
            c.draw();
        }
        ofPopStyle();
        
        for (auto a : ans)
        {
            ofVec2f field = a.wind;
            ofPushMatrix();
            ofTranslate(a.pos);
            ofSetColor(ofColor::pink);
            ofLine(0, 0, ofLerp(-windSpeed, windSpeed, field.x), ofLerp(-windSpeed, windSpeed, field.y));
            ofPopMatrix();
        }
    }
    
private:
    
    void loadAnemometer()
    {
        loadAnemometerDebug();
    }
    
    void loadAnemometerDebug()
    {
        ans.clear();
        for (int i = 0; i < 20; i++)
        {
            ofPoint pos;
            pos.set(ofRandomWidth(), ofRandomHeight());
            Anemometer a;
            a.pos = pos;
            a.wind = ofVec2f(ofRandomuf(), ofRandomuf());
            ans.push_back(a);
        }
        Anemometer lt;
        lt.pos = ofPoint(0, 0);
        lt.wind = ofVec2f(0.0, 0.0);
        Anemometer rt;
        rt.pos = ofPoint(ofGetWidth(), 0);
        rt.wind = ofVec2f(0.0, 0.0);
        Anemometer rb;
        rb.pos = ofPoint(ofGetWidth(), ofGetHeight());
        rb.wind = ofVec2f(0.0, 0.0);
        Anemometer lb;
        lb.pos = ofPoint(0, ofGetHeight());
        lb.wind = ofVec2f(0.0, 0.0);
        ans.push_back(lt);
        ans.push_back(rt);
        ans.push_back(rb);
        ans.push_back(lb);
    }
    
    void udpateWind()
    {
        loadAnemometer();
        
    }
    
    void triabgulateAnemometers()
    {
        delaunay.reset();
        for (auto a : ans)
        {
            delaunay.addPoint(a.pos);
        }
        delaunay.triangulate();
    }
    
    void getEachTriangles()
    {
        tris.clear();
        for (auto dtri : delaunay.triangles)
        {
            if (dtri.p1 < delaunay.vertices.size() &&
                dtri.p2 < delaunay.vertices.size() &&
                dtri.p3 < delaunay.vertices.size())
            {
                TriForInterp tri;
                ofPoint p1(delaunay.vertices.at(dtri.p1).x,
                           delaunay.vertices.at(dtri.p1).y,
                           0);
                ofPoint p2(delaunay.vertices.at(dtri.p2).x,
                           delaunay.vertices.at(dtri.p2).y,
                           0);
                ofPoint p3(delaunay.vertices.at(dtri.p3).x,
                           delaunay.vertices.at(dtri.p3).y,
                           0);
                tri.pts.push_back(p1);
                tri.pts.push_back(p2);
                tri.pts.push_back(p3);
                
                bool bfound = false;
                for (auto t : tris)
                {
                    if (t == tri)
                    {
                        bfound = true;
                        break;
                    }
                }
                if (!bfound)
                {
                    tris.push_back(tri);
                }
            }
        }
    }
    
    void buildConnectionLines()
    {
        connectionLines.clear();
        for (auto t : tris)
        {
            vector<ofPoint> pts = t.pts;
            ofPoint p1 = pts.at(0);
            ofPoint p2 = pts.at(1);
            ofPoint p3 = pts.at(2);
            
            // p1->p2
            ConnectionLine cl1;
            vector<WindAndPosition> cl1wp;
            Anemometer cl1a1 = getCorrespondingAnemometer(p1);
            Anemometer cl1a2 = getCorrespondingAnemometer(p2);
            WindAndPosition cl1wp1;
            cl1wp1.pos = cl1a1.pos;
            cl1wp1.wind = cl1a1.wind;
            WindAndPosition cl1wp2;
            cl1wp2.pos = cl1a2.pos;
            cl1wp2.wind = cl1a2.wind;
            cl1wp.push_back(cl1wp1);
            cl1wp.push_back(cl1wp2);
            cl1.setup(p1, p2, cl1wp, CONNECTIONLINE_SAMPLE_NUM);
            if (!isOverlappedLine(cl1))
                connectionLines.push_back(cl1);
            
            // p2->p3
            ConnectionLine cl2;
            vector<WindAndPosition> cl2wp;
            Anemometer cl2a1 = getCorrespondingAnemometer(p2);
            Anemometer cl2a2 = getCorrespondingAnemometer(p3);
            WindAndPosition cl2wp1;
            cl2wp1.pos = cl2a1.pos;
            cl2wp1.wind = cl2a1.wind;
            WindAndPosition cl2wp2;
            cl2wp2.pos = cl2a2.pos;
            cl2wp2.wind = cl2a2.wind;
            cl2wp.push_back(cl2wp1);
            cl2wp.push_back(cl2wp2);
            cl2.setup(p2, p3, cl2wp, CONNECTIONLINE_SAMPLE_NUM);
            if (!isOverlappedLine(cl2))
                connectionLines.push_back(cl2);
            
            // p3->p1
            ConnectionLine cl3;
            vector<WindAndPosition> cl3wp;
            Anemometer cl3a1 = getCorrespondingAnemometer(p3);
            Anemometer cl3a2 = getCorrespondingAnemometer(p1);
            WindAndPosition cl3wp1;
            cl3wp1.pos = cl3a1.pos;
            cl3wp1.wind = cl3a1.wind;
            WindAndPosition cl3wp2;
            cl3wp2.pos = cl3a2.pos;
            cl3wp2.wind = cl3a2.wind;
            cl3wp.push_back(cl3wp1);
            cl3wp.push_back(cl3wp2);
            cl3.setup(p3, p1, cl3wp, CONNECTIONLINE_SAMPLE_NUM);
            if (!isOverlappedLine(cl3))
                connectionLines.push_back(cl3);
        }
        
        for (auto &c : connectionLines)
        {
            c.resample();
            c.interpolateWind();
        }
    }
    
    void buildContour()
    {
        for (auto a : ans)
        {
            vector<ConnectionLine> cls = getConnectionLinesSharedAnemometer(a);
            for (int i = 0; i < CONNECTIONLINE_SAMPLE_NUM; i++)
            {
                vector<WindAndPosition> windPoss;
                for (auto cl : cls)
                {
                    if (cl.getWindPositions().size() > i)
                    {
                        if (cl.isBegin())
                        {
                            windPoss.push_back(cl.getWindPositions().at(i));
                        }
                        else
                        {
                            int idx = cl.getWindPositions().size() - i - 1;
                            windPoss.push_back(cl.getWindPositions().at(idx));
                        }
                    }
                }
                
                if (windPoss.size())
                {
                    ofPolyline line;
                    for (auto wp : windPoss)
                    {
                        line.addVertex(wp.pos);
                    }
                    line.setClosed(true);
                    center = line.getCentroid2D();
                    line.clear();
                    ofSort(windPoss, sortPosClockwise);
                    for (auto wp : windPoss)
                    {
                        line.addVertex(wp.pos);
                    }
                    line.setClosed(true);
                    Contour contour;
                    contour.setup(line, windPoss, CONTOUR_SAMPLE_NUM);
                    contour.interpolatePos();
                    contour.interpolateWind();
                    contours.push_back(contour);
                }
            }
        }
    }

    bool isOverlappedLine(ConnectionLine cl)
    {
        bool bFound = false;
        for (auto thisCL : connectionLines)
        {
            if (thisCL == cl)
            {
                bFound = true;
                break;
            }
        }
        return bFound;
    }
    
    Anemometer getCorrespondingAnemometer(ofPoint pos)
    {
        for (auto a : ans)
        {
            if (a.pos == pos)
            {
                return a;
            }
        }
    }
    
    vector<ConnectionLine> getConnectionLinesSharedAnemometer(Anemometer a)
    {
        vector<ConnectionLine> rtn;
        for (auto &c : connectionLines)
        {
            if (c.getBegin() == a.pos)
            {
                c.setBeginFrag(false);
                rtn.push_back(c);
            }
            else if (c.getEnd() == a.pos)
            {
                c.setBeginFrag(true);
                rtn.push_back(c);
            }
        }

        return rtn;
    }
    
    
    float windSpeed;
    
    vector<Anemometer> ans;
    ofxDelaunay delaunay;
    vector<TriForInterp> tris;
    vector<ConnectionLine> connectionLines;
    vector<Contour> contours;

};

#endif