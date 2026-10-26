#pragma once
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
namespace rt {
// Original code-drawn console figures. 30° isometric camera without perspective, solid faces painted back to
// front, face tone from its orientation (top lightest, right side darkest) and a single accent per figure.
struct P2 { float x, y; };
struct P3 { float x, y, z; };
class Iso {
    SDL_Renderer* renderer; float ox=0, oy=0, k=1; bool measuring=true;
    float minX=1e9f, minY=1e9f, maxX=-1e9f, maxY=-1e9f;
    std::vector<SDL_Vertex> vertices;
    static constexpr float Pi=3.14159265f;
public:
    bool grey=false; uint32_t accent=0xB699FF;
    explicit Iso(SDL_Renderer* r):renderer(r){}
    static uint32_t mix(uint32_t a,uint32_t b,float t){auto ch=[&](int n){return int(((a>>n)&255)*(1-t)+((b>>n)&255)*t+.5f);};return(ch(16)<<16)|(ch(8)<<8)|ch(0);}
    // Face tone from its outward normal: up gains light, +x (right side) loses it, +y (front) keeps the base color.
    static uint32_t lit(uint32_t c,float nx,float ny,float nz){float a=std::max(0.f,nx),b=std::max(0.f,ny),t=std::max(0.f,nz),s=a+b+t;if(s<=0)return c;float v=(t*.13f-a*.31f)/s;return v>=0?mix(c,0xFFFFFF,v):mix(c,0,-v);}
    uint32_t ink(uint32_t c)const{if(!grey)return c;int l=int(((c>>16)&255)*.30f+((c>>8)&255)*.59f+(c&255)*.11f);return(l<<16)|(l<<8)|l;}
    SDL_FPoint at(P3 p)const{return{ox+(p.x-p.y)*.8660254f*k,oy+((p.x+p.y)*.5f-p.z)*k};}
    void fan(const std::vector<P3>& pts,const std::vector<uint32_t>& colors,Uint8 alpha=255){
        if(pts.size()<3)return;
        if(measuring){for(auto& p:pts){auto s=at(p);minX=std::min(minX,s.x);maxX=std::max(maxX,s.x);minY=std::min(minY,s.y);maxY=std::max(maxY,s.y);}return;}
        vertices.clear();
        auto vertex=[&](size_t i){auto c=ink(colors[colors.size()==pts.size()?i:0]);return SDL_Vertex{at(pts[i]),{Uint8(c>>16),Uint8(c>>8),Uint8(c),alpha},{0,0}};};
        for(size_t i=1;i+1<pts.size();++i){vertices.push_back(vertex(0));vertices.push_back(vertex(i));vertices.push_back(vertex(i+1));}
        SDL_RenderGeometry(renderer,nullptr,vertices.data(),int(vertices.size()),nullptr,0);
    }
    // Measures the projected bounds first, then paints again fitted and centred in a w×h area.
    template<class F> void figure(float x,float y,float w,float h,float pad,F&& paint){
        measuring=true;ox=oy=0;k=1;minX=minY=1e9f;maxX=maxY=-1e9f;paint();
        float sw=std::max(1.f,maxX-minX),sh=std::max(1.f,maxY-minY);k=std::min((w-2*pad)/sw,(h-2*pad)/sh);
        ox=x+(w-sw*k)/2-minX*k;oy=y+(h-sh*k)/2-minY*k;measuring=false;paint();
    }
    static std::vector<P2> rrect(float x0,float y0,float x1,float y1,float r,int seg=6){
        r=std::min(r,std::min(x1-x0,y1-y0)/2);std::vector<P2> out;const P2 c[]={{x1-r,y0+r},{x1-r,y1-r},{x0+r,y1-r},{x0+r,y0+r}};
        for(int q=0;q<4;++q)for(int i=0;i<=seg;++i){float a=(q*90-90+90.f*i/seg)*Pi/180;out.push_back({c[q].x+r*std::cos(a),c[q].y+r*std::sin(a)});}
        return out;
    }
    static std::vector<P2> ellipse(float cx,float cy,float rx,float ry,int seg=40){std::vector<P2> out;for(int i=0;i<seg;++i){float a=i*2*Pi/seg;out.push_back({cx+rx*std::cos(a),cy+ry*std::sin(a)});}return out;}
    static std::vector<P2> inset(const std::vector<P2>& p,float d){
        std::vector<P2> out;size_t n=p.size();float area=0;for(size_t i=0;i<n;++i)area+=p[i].x*p[(i+1)%n].y-p[(i+1)%n].x*p[i].y;float s=area>0?1.f:-1.f;
        auto shifted=[&](size_t i,P2& a,P2& dir){P2 b=p[(i+1)%n];dir={b.x-p[i].x,b.y-p[i].y};float l=std::max(1e-5f,std::hypot(dir.x,dir.y));a={p[i].x-s*dir.y/l*d,p[i].y+s*dir.x/l*d};};
        for(size_t i=0;i<n;++i){P2 a,da,b,db;shifted((i+n-1)%n,a,da);shifted(i,b,db);float den=da.x*db.y-da.y*db.x;
            if(std::abs(den)<1e-6f){out.push_back(b);continue;}float t=((b.x-a.x)*db.y-(b.y-a.y)*db.x)/den;out.push_back({a.x+da.x*t,a.y+da.y*t});}
        return out;
    }
    void top(const std::vector<P2>& foot,float z,uint32_t c,Uint8 alpha=255){std::vector<P3> pts;for(auto& p:foot)pts.push_back({p.x,p.y,z});fan(pts,{c},alpha);}
    // Vertical extrusion of a convex footprint; only faces turned to the camera (+x/+y/+z) are painted.
    void slab(const std::vector<P2>& foot,float z0,float z1,uint32_t c,float bevel=0){
        size_t n=foot.size();float area=0;for(size_t i=0;i<n;++i)area+=foot[i].x*foot[(i+1)%n].y-foot[(i+1)%n].x*foot[i].y;float s=area>0?1.f:-1.f;
        for(size_t i=0;i<n;++i){auto a=foot[i],b=foot[(i+1)%n];float nx=s*(b.y-a.y),ny=-s*(b.x-a.x),l=std::hypot(nx,ny);if(l<1e-6f||nx+ny<=0)continue;nx/=l;ny/=l;
            fan({{a.x,a.y,z1},{b.x,b.y,z1},{b.x,b.y,z0},{a.x,a.y,z0}},{lit(c,nx,ny,0)});}
        if(bevel>0){top(foot,z1,mix(lit(c,0,0,1),0xFFFFFF,.24f));top(inset(foot,bevel),z1,lit(c,0,0,1));}else top(foot,z1,lit(c,0,0,1));
    }
    void box(float x0,float y0,float z0,float x1,float y1,float z1,uint32_t c,float bevel=0){slab({{x0,y0},{x1,y0},{x1,y1},{x0,y1}},z0,z1,c,bevel);}
    // Extrusion along x of a convex (y,z) profile, for slanted parts such as an arcade cabinet.
    void prismX(const std::vector<P2>& profile,float x0,float x1,uint32_t c){
        size_t n=profile.size();float area=0;for(size_t i=0;i<n;++i)area+=profile[i].x*profile[(i+1)%n].y-profile[(i+1)%n].x*profile[i].y;float s=area>0?1.f:-1.f;
        for(size_t i=0;i<n;++i){auto a=profile[i],b=profile[(i+1)%n];float ny=s*(b.y-a.y),nz=-s*(b.x-a.x),l=std::hypot(ny,nz);if(l<1e-6f||ny+nz<=0)continue;ny/=l;nz/=l;
            fan({{x0,a.x,a.y},{x1,a.x,a.y},{x1,b.x,b.y},{x0,b.x,b.y}},{lit(c,0,ny,nz)});}
        std::vector<P3> cap;for(auto& p:profile)cap.push_back({x1,p.x,p.y});fan(cap,{lit(c,1,0,0)});
    }
    // Flat shapes lying on the front (y), right (x) or an arbitrary plane.
    void front(const std::vector<P2>& xz,float y,uint32_t c){std::vector<P3> pts;for(auto& p:xz)pts.push_back({p.x,y,p.y});fan(pts,{lit(c,0,1,0)});}
    void right(const std::vector<P2>& yz,float x,uint32_t c){std::vector<P3> pts;for(auto& p:yz)pts.push_back({x,p.x,p.y});fan(pts,{lit(c,1,0,0)});}
    void disc(P3 o,P3 u,P3 v,float r,uint32_t c,int seg=28){std::vector<P3> pts;for(int i=0;i<seg;++i){float a=i*2*Pi/seg,cu=r*std::cos(a),sv=r*std::sin(a);pts.push_back({o.x+u.x*cu+v.x*sv,o.y+u.y*cu+v.y*sv,o.z+u.z*cu+v.z*sv});}fan(pts,{c});}
    void cylinder(float cx,float cy,float r,float z0,float z1,uint32_t c,float bevel=0){slab(ellipse(cx,cy,r,r,32),z0,z1,c,bevel);}
    // Round ball: a screen-space disc whose centre vertex is lit from the upper left.
    void ball(P3 o,float r,uint32_t c){
        if(measuring){fan({{o.x-r,o.y-r,o.z},{o.x+r,o.y+r,o.z},{o.x,o.y,o.z+r},{o.x,o.y,o.z-r}},{c});return;}
        auto s=at(o);float rr=r*k;vertices.clear();auto hi=ink(mix(c,0xFFFFFF,.38f)),lo=ink(mix(c,0,.32f));
        SDL_Vertex centre{{s.x-rr*.3f,s.y-rr*.35f},{Uint8(hi>>16),Uint8(hi>>8),Uint8(hi),255},{0,0}};
        for(int i=0;i<32;++i){float a0=i*2*Pi/32,a1=(i+1)*2*Pi/32;SDL_Vertex e0{{s.x+rr*std::cos(a0),s.y+rr*std::sin(a0)},{Uint8(lo>>16),Uint8(lo>>8),Uint8(lo),255},{0,0}},e1=e0;e1.position={s.x+rr*std::cos(a1),s.y+rr*std::sin(a1)};vertices.push_back(centre);vertices.push_back(e0);vertices.push_back(e1);}
        SDL_RenderGeometry(renderer,nullptr,vertices.data(),int(vertices.size()),nullptr,0);
    }
    // Cable resting on the floor: a ribbon of quads along a polyline.
    void cable(const std::vector<P2>& path,float width,uint32_t c){
        for(size_t i=0;i+1<path.size();++i){auto a=path[i],b=path[i+1];float dx=b.x-a.x,dy=b.y-a.y,l=std::max(1e-5f,std::hypot(dx,dy)),px=-dy/l*width/2,py=dx/l*width/2;
            fan({{a.x+px,a.y+py,.12f},{b.x+px,b.y+py,.12f},{b.x-px,b.y-py,.12f},{a.x-px,a.y-py,.12f}},{lit(c,0,0,1)});}
    }
    void shadow(const std::vector<P2>& foot,float dx,float dy,Uint8 alpha){std::vector<P3> pts;for(auto& p:foot)pts.push_back({p.x+dx,p.y+dy,0});fan(pts,{0},alpha);}
    // Seam or rule line on a face; width in world units, drawn as a screen-space ribbon.
    void stroke(P3 a,P3 b,float width,uint32_t c){
        if(measuring){fan({a,b,a},{c});return;}
        auto s=at(a),e=at(b);float dx=e.x-s.x,dy=e.y-s.y,l=std::max(1e-4f,std::hypot(dx,dy)),h=std::max(.6f,width*k/2),nx=-dy/l*h,ny=dx/l*h;
        auto col=ink(c);SDL_Color sc{Uint8(col>>16),Uint8(col>>8),Uint8(col),255};
        SDL_Vertex v[6]={{{s.x+nx,s.y+ny},sc,{0,0}},{{e.x+nx,e.y+ny},sc,{0,0}},{{e.x-nx,e.y-ny},sc,{0,0}},{{s.x+nx,s.y+ny},sc,{0,0}},{{e.x-nx,e.y-ny},sc,{0,0}},{{s.x-nx,s.y-ny},sc,{0,0}}};
        SDL_RenderGeometry(renderer,nullptr,v,6,nullptr,0);
    }
    // Rubber feet peeking out under the three visible corners of a body.
    void feet(float x0,float y0,float x1,float y1,float inset=2.4f){for(auto p:{P2{x1-inset,y0+inset},P2{x0+inset,y1-inset},P2{x1-inset,y1-inset}})cylinder(p.x,p.y,1.5f,-.7f,0,0x121115);}
    // Closed vent slots on the right (x) face: count slots of width w every pitch along y.
    void ventsRight(float x,float y0,float pitch,int count,float w,float z0,float z1,uint32_t c){for(int i=0;i<count;++i){float y=y0+i*pitch;right(rrect(y,z0,y+w,z1,w/2,3),x,c);}}
    // Connector plug at the end of a cable, sitting against a front (y) face.
    void plug(float x,float y,float z,uint32_t c){box(x-.75f,y,z-.55f,x+.75f,y+1.1f,z+.55f,c);}

    // ---- Figures ----
    void nes(){
        const uint32_t shell=0xD3D0CA,band=0x2F2E34,door=0xC2BFB9,port=0x17161B,red=accent;
        shadow(rrect(-.6f,-.6f,26.4f,20.4f,1.2f),1.6f,.8f,70);shadow(rrect(16.4f,22.6f,30.8f,29.4f,1),.8f,.4f,60);feet(0,0,25,19);
        box(0,0,0,25,9,8.3f,shell,.22f);ventsRight(25.01f,1.2f,1.05f,7,.42f,4.2f,7.2f,mix(shell,0,.32f));
        top(rrect(15.5f,1.6f,22.5f,4.2f,.4f),8.31f,mix(shell,0,.1f));stroke({16.3f,2.4f,8.32f},{21.f,2.4f,8.32f},.22f,mix(shell,0,.35f));stroke({16.3f,3.3f,8.32f},{19.4f,3.3f,8.32f},.22f,mix(shell,0,.35f));
        for(float x:{1.6f,13.4f})top(ellipse(x,1.6f,.32f,.32f,12),8.31f,mix(shell,0,.45f));box(0,9,0,25,19,7.6f,shell,.22f);
        for(int i=0;i<7;++i){float y=10.4f+i*1.15f;top({{2.2f,y},{22.8f,y},{22.8f,y+.34f},{2.2f,y+.34f}},7.6f,mix(shell,0,.2f));top({{2.2f,y+.34f},{22.8f,y+.34f},{22.8f,y+.5f},{2.2f,y+.5f}},7.6f,mix(shell,0xFFFFFF,.35f));}
        front({{0,0},{25,0},{25,3.3f},{0,3.3f}},19,band);right({{9,0},{19,0},{19,3.3f},{9,3.3f}},25,band);
        front({{4.6f,3.9f},{20.4f,3.9f},{20.4f,7.1f},{4.6f,7.1f}},19,door);front({{4.6f,6.3f},{20.4f,6.3f},{20.4f,6.5f},{4.6f,6.5f}},19,mix(door,0,.25f));
        front({{11.2f,4.3f},{13.8f,4.3f},{13.8f,4.75f},{11.2f,4.75f}},19,mix(door,0,.35f));
        box(1.6f,19,.8f,4.8f,19.5f,2.3f,shell);box(5.4f,19,.8f,8.6f,19.5f,2.3f,shell);front(rrect(9.5f,1.4f,10.4f,1.8f,.2f),19,red);
        for(float x:{14.6f,18.2f}){front(rrect(x-.25f,.55f,x+2.65f,2.85f,.5f),19,0x5A5960);front(rrect(x,.8f,x+2.4f,2.6f,.4f),19,port);for(int i=0;i<3;++i)front(rrect(x+.55f+i*.55f,1.6f,x+.8f+i*.55f,1.85f,.1f),19,0x6E6D74);}
        cable({{19.2f,22.8f},{18.4f,21.6f},{16.8f,20.6f},{15.8f,19.9f},{15.8f,19.2f}},.36f,0x232227);plug(15.8f,19,1.7f,0x2A292E);box(18.7f,22.3f,0,19.7f,22.8f,.7f,0x2A292E);
        box(16.6f,22.8f,0,30.4f,29.2f,1.2f,shell,.18f);top(rrect(17.3f,23.4f,29.7f,28.6f,.3f),1.2f,band);
        top(rrect(18.2f,25.3f,21.4f,26.7f,.2f),1.21f,0x8E8C92);top(rrect(19.1f,24.4f,20.5f,27.6f,.2f),1.21f,0x8E8C92);
        box(18.4f,25.5f,1.2f,21.2f,26.5f,1.55f,0x101014);box(19.3f,24.6f,1.2f,20.3f,27.4f,1.55f,0x101014);
        for(float x:{22.6f,24.3f})top(rrect(x,26.2f,x+1.3f,26.6f,.2f),1.21f,0x5F5E66);
        for(float x:{26.5f,28.5f}){top(rrect(x-.95f,25.4f,x+.95f,27.6f,.25f),1.21f,0xDAD7D2);cylinder(x,26.5f,.72f,1.2f,1.6f,red);}
    }
    void snes(){
        const uint32_t shell=0xD8D5DF,deck=0xC3BFCC,slider=0x9E97B6,port=0x1B1A20,lav=mix(accent,0xFFFFFF,.45f);
        shadow(rrect(-.6f,-.6f,24.6f,21.6f,2.6f),1.6f,.8f,70);shadow(rrect(16.4f,23.6f,31.6f,29.8f,2.6f),.8f,.4f,60);feet(0,0,24,21,2.6f);
        slab(rrect(0,0,24,21,2.4f),0,5,shell,.24f);ventsRight(24.01f,6.f,1.f,6,.42f,1.2f,3.4f,mix(shell,0,.3f));top(rrect(1.2f,1.2f,22.8f,19.8f,1.8f),5,deck);front({{2.4f,3.9f},{21.6f,3.9f},{21.6f,4.1f},{2.4f,4.1f}},21,mix(shell,0,.18f));
        slab(rrect(6,2.2f,18,11.2f,1.2f),5,7.2f,shell,.2f);top({{7.6f,5.1f},{16.4f,5.1f},{16.4f,6.9f},{7.6f,6.9f}},7.2f,port);top({{7.6f,5.1f},{16.4f,5.1f},{16.4f,5.55f},{7.6f,5.55f}},7.2f,0x45434C);
        box(8.f,5.25f,7.2f,16.f,6.75f,11.6f,0x8F8AA0,.15f);front(rrect(9.f,8.2f,15.f,11.f,.3f),6.75f,0xE4E0EE);front(rrect(9.6f,8.7f,12.6f,9.f,.1f),6.75f,0xA9A3BE);
        for(float x:{3.f,16.8f}){top(rrect(x-.3f,13.3f,x+4.5f,16.5f,.9f),5,mix(deck,0,.3f));slab(rrect(x,13.6f,x+2.6f,16.2f,.7f),5,5.75f,slider,.15f);}
        cylinder(12,15,1.25f,5,5.7f,accent,.12f);
        for(float x:{4.2f,9.2f}){front(rrect(x-.25f,.95f,x+3.25f,3.15f,.6f),21,0x5C5A64);front(rrect(x,1.2f,x+3,2.9f,.5f),21,port);for(int i=0;i<4;++i)front(rrect(x+.6f+i*.5f,1.95f,x+.82f+i*.5f,2.17f,.1f),21,0x76737F);}
        cable({{18.2f,24.6f},{14.f,24.2f},{8.6f,23.4f},{6.f,22.2f},{5.7f,21.2f}},.36f,0x2A2930);plug(5.7f,21,2.05f,0x2A2930);
        slab(ellipse(19.6f,26.7f,2.9f,2.9f,32),0,1.3f,shell,.18f);box(19.6f,23.8f,0,27.6f,29.6f,1.3f,shell,.18f);slab(ellipse(27.6f,26.7f,2.9f,2.9f,32),0,1.3f,shell,.18f);
        top(ellipse(27.6f,26.7f,2.25f,2.25f,32),1.3f,mix(shell,0,.16f));
        box(18.5f,26.3f,1.3f,20.7f,27.1f,1.6f,0x2A2930);box(19.2f,25.6f,1.3f,20.f,27.8f,1.6f,0x2A2930);
        for(float x:{22.5f,24.3f})top(rrect(x,26.9f,x+1.2f,27.35f,.22f),1.31f,0x6A6774);
        cylinder(27.6f,25.45f,.52f,1.3f,1.62f,lav);cylinder(26.35f,26.7f,.52f,1.3f,1.62f,lav);cylinder(28.85f,26.7f,.52f,1.3f,1.62f,accent);cylinder(27.6f,27.95f,.52f,1.3f,1.62f,accent);
    }
    void megaDrive(){
        const uint32_t shell=0x2E2D34,disc=0x38363F,trim=0x4D4B55,port=0x111014;
        shadow(rrect(-.6f,-.6f,27.6f,19.6f,1.6f),1.6f,.8f,80);shadow(rrect(16.4f,22.4f,30.6f,28.8f,3),.8f,.4f,70);feet(0,0,27,19);
        slab(rrect(0,0,27,19,1.5f),0,4.4f,shell,.2f);ventsRight(27.01f,4.f,1.f,8,.4f,1.f,3.f,0x17161B);front(ellipse(24.5f,2.f,.42f,.42f,16),19,port);front({{1.5f,3.1f},{25.5f,3.1f},{25.5f,3.25f},{1.5f,3.25f}},19,trim);
        top(ellipse(10.5f,9.5f,7.9f,7.9f,48),4.4f,0x1D1C21);slab(ellipse(10.5f,9.5f,7.5f,7.5f,48),4.4f,5.2f,disc,.18f);
        for(float r:{6.3f,4.6f}){top(ellipse(10.5f,9.5f,r,r,48),5.2f,mix(disc,0,.3f));top(ellipse(10.5f,9.5f,r-.25f,r-.25f,48),5.2f,lit(disc,0,0,1));}
        slab(rrect(6.6f,7.4f,14.4f,11.6f,.6f),5.2f,6.2f,0x29282E,.14f);top({{7.4f,9.05f},{13.6f,9.05f},{13.6f,10.f},{7.4f,10.f}},6.2f,port);top({{7.4f,9.05f},{13.6f,9.05f},{13.6f,9.4f},{7.4f,9.4f}},6.2f,0x403E47);
        box(7.7f,9.1f,6.2f,13.3f,9.95f,10.4f,0x26252B,.12f);front(rrect(8.4f,7.f,12.6f,9.6f,.3f),9.95f,0x5B5866);
        top(rrect(19.8f,3.4f,24.6f,6.8f,.8f),4.4f,0x1A191E);slab(rrect(20.2f,3.8f,22.6f,6.4f,.6f),4.4f,5,trim,.12f);
        cylinder(22.2f,11.8f,1.05f,4.4f,4.9f,trim,.12f);top(ellipse(20.6f,15.6f,.42f,.42f,20),4.41f,accent);
        for(int i=0;i<4;++i){float x=19.9f+i*1.2f;top({{x,14.9f},{x+.55f,14.9f},{x+.55f,16.4f},{x,16.4f}},4.41f,0x1D1C21);}
        for(float x:{3.2f,7.8f}){front(rrect(x-.25f,.75f,x+3.25f,2.75f,.5f),19,trim);front(rrect(x,1.f,x+3,2.5f,.4f),19,port);}
        cable({{17.8f,24.4f},{13.2f,23.8f},{7.6f,22.4f},{4.8f,20.6f},{4.6f,19.2f}},.36f,0x1A191E);plug(4.7f,19,1.75f,0x141317);
        slab(rrect(16.6f,22.6f,30.4f,28.6f,3),0,1.35f,shell,.18f);
        slab(ellipse(19.8f,25.6f,2.05f,2.05f,32),1.35f,1.45f,trim);box(18.6f,25.25f,1.45f,21.f,25.95f,1.75f,0x19181D);box(19.45f,24.4f,1.45f,20.15f,26.8f,1.75f,0x19181D);
        cylinder(25.6f,24.25f,.42f,1.35f,1.55f,trim);
        for(int i=0;i<3;++i)cylinder(24.3f+i*1.95f,26.9f-i*.75f,.82f,1.35f,1.75f,0x4F4D57,.1f);
    }
    void playStation(){
        const uint32_t shell=0xC9C8CE,lid=0xD5D4D9,trim=0xA9A8B0,port=0x1C1B21;
        shadow(rrect(-.6f,-.6f,27.6f,19.6f,1.4f),1.6f,.8f,70);shadow(rrect(16.6f,22.6f,30.2f,30.6f,2),.8f,.4f,60);feet(0,0,27,19);
        slab(rrect(0,0,27,19,1.2f),0,4.8f,shell,.2f);ventsRight(27.01f,3.f,1.6f,9,.5f,1.2f,3.6f,mix(shell,0,.3f));front({{1.2f,3.6f},{25.8f,3.6f},{25.8f,3.75f},{1.2f,3.75f}},19,mix(shell,0,.2f));
        top(ellipse(9.6f,9.5f,7.9f,7.9f,48),4.8f,mix(shell,0,.32f));slab(ellipse(9.6f,9.5f,7.6f,7.6f,48),4.8f,5.25f,lid,.2f);
        top(ellipse(9.6f,9.5f,2.6f,2.6f,40),5.25f,mix(lid,0,.08f));top(ellipse(9.6f,9.5f,.5f,.5f,20),5.25f,mix(lid,0,.22f));
        top({{18.2f,1.2f},{18.45f,1.2f},{18.45f,17.8f},{18.2f,17.8f}},4.8f,mix(shell,0,.22f));
        cylinder(22.4f,5.6f,1.9f,4.8f,5.3f,trim,.18f);cylinder(20.6f,12.6f,1.05f,4.8f,5.25f,trim,.12f);cylinder(24.2f,12.6f,1.05f,4.8f,5.25f,trim,.12f);top(ellipse(22.4f,15.9f,.42f,.42f,20),4.81f,accent);
        for(float x:{3.8f,9.6f}){front(rrect(x-.25f,1.9f,x+3.65f,3.25f,.4f),19,0x6D6C74);front(rrect(x,2.1f,x+3.4f,3.05f,.3f),19,port);front({{x,.7f},{x+3.4f,.7f},{x+3.4f,1.05f},{x,1.05f}},19,port);}
        cable({{20.2f,23.6f},{14.f,22.8f},{7.8f,21.8f},{5.6f,20.4f},{5.5f,19.2f}},.36f,0x2C2B31);plug(5.5f,19,2.6f,0x2C2B31);
        slab(rrect(17.2f,23,29.6f,27.6f,1.8f),0,1.4f,shell,.18f);slab(ellipse(19.f,28.4f,1.9f,2.2f,28),0,1.4f,shell,.16f);slab(ellipse(27.8f,28.4f,1.9f,2.2f,28),0,1.4f,shell,.16f);
        box(18.1f,24.95f,1.4f,20.5f,25.65f,1.62f,0x55545C);box(18.95f,24.1f,1.4f,19.65f,26.5f,1.62f,0x55545C);
        for(auto p:{P2{27.2f,24.3f},P2{28.3f,25.3f},P2{27.2f,26.3f},P2{26.1f,25.3f}})cylinder(p.x,p.y,.44f,1.4f,1.62f,0x56555D);
        for(float x:{21.9f,24.9f}){cylinder(x,26.9f,.95f,1.4f,1.5f,0x3E3D44);cylinder(x,26.9f,.62f,1.5f,2.2f,0x2C2B31,.1f);}
    }
    void arcade(){
        const uint32_t cab=0x3B3449,panel=0x2B2635,screen=0x14121C,trim=0x5A516B,glow=accent;
        shadow({{-.4f,-.4f},{11.6f,-.4f},{11.6f,12.4f},{-.4f,12.4f}},2.2f,1,80);for(auto p:{P2{10.2f,.8f},P2{.8f,7.8f},P2{10.2f,7.8f}})cylinder(p.x,p.y,.55f,-.5f,0,0x121115);
        prismX({{0,0},{8.6f,0},{8.6f,11.6f},{0,11.6f}},0,11,cab);
        front({{3.2f,2.6f},{7.8f,2.6f},{7.8f,7.4f},{3.2f,7.4f}},8.6f,0x2A2534);for(float x:{4.2f,6.f})front({{x,5.6f},{x+.8f,5.6f},{x+.8f,6.6f},{x,6.6f}},8.6f,mix(glow,0,.35f));
        front({{0,0},{11,0},{11,.9f},{0,.9f}},8.6f,0x221E2A);for(int i=0;i<8;++i)front(ellipse(2.f+i*1.f,9.6f,.2f,.2f,10),8.6f,0x1A1722);
        prismX({{0,11.6f},{6.6f,11.6f},{6.6f,14.f},{5.2f,21.6f},{5.2f,23.f},{0,23.f}},0,11,cab);
        // Slanted screen: inset on the plane through (y=6.6,z=14) and (y=5.2,z=21.6).
        auto onScreen=[](float x,float t,float lift){return P3{x,6.6f-1.4f*t+lift*.98f,14.f+7.6f*t+lift*.18f};};
        fan({onScreen(.9f,.06f,.02f),onScreen(10.1f,.06f,.02f),onScreen(10.1f,.94f,.02f),onScreen(.9f,.94f,.02f)},{0x0D0C12});
        fan({onScreen(1.5f,.14f,.04f),onScreen(9.5f,.14f,.04f),onScreen(9.5f,.88f,.04f),onScreen(1.5f,.88f,.04f)},{mix(glow,screen,.55f),mix(glow,screen,.55f),mix(glow,0xFFFFFF,.15f),mix(glow,0xFFFFFF,.15f)});
        for(int i=0;i<5;++i){float t=.22f+i*.13f;fan({onScreen(1.5f,t,.05f),onScreen(9.5f,t,.05f),onScreen(9.5f,t+.025f,.05f),onScreen(1.5f,t+.025f,.05f)},{mix(glow,0xFFFFFF,.4f)},90);}
        prismX({{0,23.f},{7.4f,23.f},{7.4f,26.6f},{0,26.6f}},0,11,cab);front({{.6f,23.5f},{10.4f,23.5f},{10.4f,26.1f},{.6f,26.1f}},7.4f,mix(glow,0xFFFFFF,.55f));
        prismX({{6.f,11.6f},{11.6f,11.6f},{11.6f,12.7f},{6.f,14.4f}},0,11,panel);
        // Control panel top: plane from (y=11.6,z=12.7) back to (y=6,z=14.4).
        auto onPanel=[](float x,float t){return P3{x,11.6f-5.6f*t,12.7f+1.7f*t};};P3 u{1,0,0},v{0,-.957f,.29f};
        auto stick=onPanel(3.f,.45f);disc(stick,u,v,.9f,0x18161E);fan({{stick.x-.14f,stick.y,stick.z},{stick.x+.14f,stick.y,stick.z},{stick.x+.14f,stick.y,stick.z+1.7f},{stick.x-.14f,stick.y,stick.z+1.7f}},{0x9C98A6});ball({stick.x,stick.y,stick.z+1.9f},.6f,glow);
        for(int i=0;i<3;++i)for(int j=0;j<2;++j){auto c=onPanel(6.1f+i*1.35f,.3f+j*.32f);disc(c,u,v,.5f,0x15131A);disc({c.x,c.y,c.z+.12f},u,v,.42f,j?trim:mix(trim,0xFFFFFF,.25f));}
        right({{1.2f,2.f},{7.6f,2.f},{6.2f,20.f},{1.2f,20.f}},11.01f,mix(cab,0xFFFFFF,.08f));
        for(auto seg:{std::pair<P3,P3>{{11.02f,8.6f,.9f},{11.02f,8.6f,11.6f}},{{11.02f,6.6f,14.f},{11.02f,5.2f,21.6f}},{{11.02f,5.2f,21.6f},{11.02f,5.2f,23.f}},{{11.02f,7.4f,23.f},{11.02f,7.4f,26.6f}}})stroke(seg.first,seg.second,.28f,trim);right({{2.f,3.f},{6.8f,3.f},{5.7f,10.f},{2.f,10.f}},11.02f,mix(cab,glow,.22f));
    }
    void paint(const std::string& system){
        if(system=="NES")nes();else if(system=="Mega Drive")megaDrive();else if(system=="PlayStation")playStation();else if(system=="Arcade")arcade();else snes();
    }
};
// Figures are painted once into a supersampled target and reused; linear downscaling gives antialiased edges on GLES2.
class ConsoleArt {
    struct Entry { std::string key; SDL_Texture* texture; size_t bytes; };
    SDL_Renderer* renderer; std::vector<Entry> entries; size_t budget;
public:
    explicit ConsoleArt(SDL_Renderer* r,size_t budgetBytes=size_t(6)<<20):renderer(r),budget(budgetBytes){}
    void clear(){for(auto& e:entries)SDL_DestroyTexture(e.texture);entries.clear();}
    void draw(const std::string& system,int x,int y,int w,int h,bool grey,uint32_t accent){
        if(w<16||h<16)return;
        int qw=(w+15)/16*16,qh=(h+15)/16*16;float sx=1,sy=1;SDL_RenderGetScale(renderer,&sx,&sy);float ss=std::clamp(std::max(sx,sy)*2,2.f,3.f);
        auto key=system+(grey?"|g|":"|c|")+std::to_string(accent)+"|"+std::to_string(qw)+"x"+std::to_string(qh);
        auto it=std::find_if(entries.begin(),entries.end(),[&](const Entry& e){return e.key==key;});
        if(it==entries.end()){
            int tw=int(qw*ss),th=int(qh*ss);SDL_Texture* texture=SDL_RenderTargetSupported(renderer)?SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_TARGET,tw,th):nullptr;
            Iso iso(renderer);iso.grey=grey;iso.accent=accent;
            if(!texture){SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);iso.figure(float(x),float(y),float(w),float(h),std::min(w,h)*.05f,[&]{iso.paint(system);});SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);return;}
            auto* previous=SDL_GetRenderTarget(renderer);SDL_SetRenderTarget(renderer,texture);
            SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);SDL_SetRenderDrawColor(renderer,0,0,0,0);SDL_RenderClear(renderer);
            SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);iso.figure(0,0,float(tw),float(th),std::min(tw,th)*.05f,[&]{iso.paint(system);});
            SDL_SetRenderTarget(renderer,previous);SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);SDL_SetTextureScaleMode(texture,SDL_ScaleModeLinear);
            // Blending onto a cleared target leaves premultiplied colour; composite it as such to avoid dark fringes.
            auto premultiplied=SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ONE,SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,SDL_BLENDOPERATION_ADD,SDL_BLENDFACTOR_ONE,SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,SDL_BLENDOPERATION_ADD);
            if(SDL_SetTextureBlendMode(texture,premultiplied)!=0)SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND);
            entries.push_back({key,texture,size_t(tw)*th*4});size_t total=0;for(auto& e:entries)total+=e.bytes;
            while(total>budget&&entries.size()>1){total-=entries.front().bytes;SDL_DestroyTexture(entries.front().texture);entries.erase(entries.begin());}
            it=entries.end()-1;
        }else if(it!=entries.end()-1){auto e=*it;entries.erase(it);entries.push_back(e);it=entries.end()-1;}
        SDL_Rect dst{x+(w-qw)/2,y+(h-qh)/2,qw,qh};SDL_RenderCopy(renderer,it->texture,nullptr,&dst);
    }
};
}
