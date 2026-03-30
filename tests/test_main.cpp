#include "KonEngine.hpp"
#include "collision/collision_world.hpp"
#include "node/collider2d.hpp"
#include <iostream>
#include <cmath>
#include <string>
#include <vector>
#include <memory>

// ─────────────────────────────────────────────────────────────────────────────
// Test runner
// ─────────────────────────────────────────────────────────────────────────────
static int s_pass=0, s_fail=0;
#define TEST(name,expr) do{ \
    if(expr){std::cout<<"  [PASS] "<<name<<"\n";s_pass++;} \
    else    {std::cout<<"  [FAIL] "<<name<<"\n";s_fail++;} \
}while(0)
#define SECTION(n) std::cout<<"\n-- "<<n<<" --\n"

static bool near(float a,float b){return std::fabs(a-b)<0.001f;}
static bool nearv(Vector2 a,Vector2 b){return near(a.x,b.x)&&near(a.y,b.y);}
static float sumDur(const Animation& a){float d=0;for(auto&f:a.frames)d+=f.duration;return d;}

// ─────────────────────────────────────────────────────────────────────────────
// HEADLESS TESTS
// ─────────────────────────────────────────────────────────────────────────────
void test_vector2(){
    SECTION("Vector2");
    Vector2 a(3,4),b(1,2);
    TEST("add",   nearv(a+b,{4,6}));   TEST("sub", nearv(a-b,{2,2}));
    TEST("mul",   nearv(a*2,{6,8}));   TEST("div", nearv(a/2,{1.5f,2}));
    TEST("neg",   nearv(-a,{-3,-4}));  TEST("eq",  a==Vector2(3,4));
    TEST("length",near(a.Length(),5)); TEST("dist",near(a.Distance({}),5));
    Vector2 n=a.Normalized();
    TEST("norm len",near(n.Length(),1)); TEST("norm dir",near(n.x,0.6f)&&near(n.y,0.8f));
    TEST("dot perp",near(Vector2::Right().Dot(Vector2::Up()),0));
    float pi=3.14159265f;
    Vector2 r90=Vector2::Right().Rotated(pi/2);
    TEST("rot 90", near(r90.x,0)&&near(r90.y,1));
    TEST("rot len",near(Vector2(3,4).Rotated(1.23f).Length(),5));
    Vector2 ref=Vector2(1,1).Reflected({0,-1});
    TEST("reflect floor Y",near(ref.x,1)&&near(ref.y,-1));
    Vector2 wr=Vector2(1,0).Reflected(Vector2(-1,-1).Normalized());
    TEST("reflect 45 dir",near(wr.x,0)&&near(wr.y,-1));
    TEST("lerp mid",nearv(Vector2::Lerp({0,0},{10,20},0.5f),{5,10}));
    TEST("Zero",Vector2::Zero()==Vector2(0,0)); TEST("Up",Vector2::Up()==Vector2(0,-1));
}

void test_primitives(){
    SECTION("Primitive collision");
    TEST("circles touch",!CheckCollisionCircles({0,0,50},{100,0,50}));
    TEST("circles overlap",CheckCollisionCircles({0,0,50},{99,0,50}));
    TEST("circles diag",CheckCollisionCircles({0,0,3},{3,4,3}));
    Rectangle r(100,100,200,100);
    TEST("circ inside",CheckCollisionCircleRec({200,150,10},r));
    TEST("circ touch",!CheckCollisionCircleRec({90,150,10},r));
    TEST("circ 1px",CheckCollisionCircleRec({91,150,10},r));
    TEST("recs overlap",CheckCollisionRecs({0,0,100,100},{25,25,50,50}));
    TEST("recs touch",!CheckCollisionRecs({0,0,100,100},{100,0,100,100}));
    TEST("recs 1px",CheckCollisionRecs({0,0,100,100},{99,0,100,100}));
}

void test_sat(){
    SECTION("SAT");
    Collider2D ca("ca"),cb("cb");
    ca.shape=ColliderShape::Circle;ca.x=0;ca.y=0;ca.radius=50;
    cb.shape=ColliderShape::Circle;cb.x=80;cb.y=0;cb.radius=50;
    TEST("circ overlap",CollisionWorld::Overlaps(&ca,&cb));
    cb.x=100; TEST("exact touch",!CollisionWorld::Overlaps(&ca,&cb));
    Collider2D r1("r1"),r2("r2");
    r1.shape=ColliderShape::Rectangle;r1.x=0;r1.y=0;r1.width=100;r1.height=100;r1.originX=0;r1.originY=0;
    r2.shape=ColliderShape::Rectangle;r2.x=50;r2.y=50;r2.width=100;r2.height=100;r2.originX=0;r2.originY=0;
    TEST("rect overlap",CollisionWorld::Overlaps(&r1,&r2));
    r2.x=200; TEST("rect separate",!CollisionWorld::Overlaps(&r1,&r2));
    Collider2D tri("tri"),pt("pt");
    tri.shape=ColliderShape::Custom;tri.x=0;tri.y=0;tri.points={{0,-60},{52,30},{-52,30}};
    pt.shape=ColliderShape::Rectangle;pt.width=10;pt.height=10;pt.originX=0.5f;pt.originY=0.5f;
    pt.x=0;pt.y=0; TEST("poly in tri",CollisionWorld::Overlaps(&tri,&pt));
    pt.x=0;pt.y=-80; TEST("poly above",!CollisionWorld::Overlaps(&tri,&pt));
}

void test_layer_mask(){
    SECTION("Layer/mask");
    auto hits=[](uint32_t la,uint32_t ma,uint32_t lb,uint32_t mb){return (la&mb)||(lb&ma);};
    TEST("default",hits(1,1,1,1));
    TEST("1,2 vs 2,1",hits(1,2,2,1));
    TEST("1,4 vs 2,8 miss",!hits(1,4,2,8));
    Collider2D a("a"),b("b");
    a.x=0;a.y=0;a.width=100;a.height=100;a.originX=0;a.originY=0;
    b.x=0;b.y=0;b.width=100;b.height=100;b.originX=0;b.originY=0;
    a.layer=1;a.mask=4; b.layer=2;b.mask=8;
    int entered=0;
    a.Connect("on_collision_enter",[&](Collider2D*){entered++;});
    CollisionWorld world;world.Add(&a);world.Add(&b);
    world.Update(); TEST("mismatch suppressed",entered==0);
    b.layer=4;b.mask=1;
    world.Update(); TEST("match fires",entered==1);
}

void test_contacts(){
    SECTION("IsColliding() + GetContacts()");
    Collider2D a("a"),b("b"),c("c");
    auto setup=[](Collider2D& x,float px){
        x.shape=ColliderShape::Rectangle;x.x=px;x.y=0;
        x.width=80;x.height=80;x.originX=0;x.originY=0;
    };
    setup(a,0); setup(b,20); setup(c,40);
    CollisionWorld world; world.Add(&a);world.Add(&b);world.Add(&c);
    world.Update();
    TEST("a IsColliding()",a.IsColliding());
    TEST("a has 2 contacts",a.GetContacts().size()==2);
    bool gotB=false,gotC=false;
    for(auto* ct:a.GetContacts()){ if(ct==&b)gotB=true; if(ct==&c)gotC=true; }
    TEST("contact is b",gotB); TEST("contact is c",gotC);
    c.x=500; world.Update();
    TEST("still colliding after c leaves",a.IsColliding());
    TEST("1 contact left",a.GetContacts().size()==1);
    TEST("remaining is b",a.GetContacts()[0]==&b);
    b.x=500; world.Update();
    TEST("not colliding",!a.IsColliding());
    TEST("contacts empty",a.GetContacts().empty());
}

void test_signals(){
    SECTION("Signals + events");
    CollisionWorld w;
    Collider2D p("p"),q("q");
    p.x=0;p.y=0;p.width=50;p.height=50;p.originX=0;p.originY=0;
    q.x=0;q.y=0;q.width=50;q.height=50;q.originX=0;q.originY=0;
    w.Add(&p);w.Add(&q);
    int en=0,ex=0;
    p.Connect("on_collision_enter",[&](Collider2D*){en++;});
    p.Connect("on_collision_exit", [&](Collider2D*){ex++;});
    w.Update(); TEST("enter",en==1);
    w.Update(); TEST("no dupe",en==1);
    q.x=500; w.Update(); TEST("exit",ex==1);
    q.x=0;   w.Update(); TEST("re-enter",en==2);
    TEST("IsColliding after re-enter",p.IsColliding());
}

void test_scene(){
    SECTION("Scene");
    Scene s;
    auto* a=s.Add<Collider2D>("a");
    auto* b=s.Add<Collider2D>("b");
    a->x=0;a->y=0;a->width=100;a->height=100;a->originX=0;a->originY=0;
    b->x=0;b->y=0;b->width=100;b->height=100;b->originX=0;b->originY=0;
    int en=0,ex=0;
    a->Connect("on_collision_enter",[&](Collider2D*){en++;});
    a->Connect("on_collision_exit", [&](Collider2D*){ex++;});
    s.Update(0.016f);
    TEST("scene detects collision",en==1);
    TEST("IsColliding in scene",a->IsColliding());
    s.Remove("b");
    TEST("removed node gone",s.GetNode("b")==nullptr);
    s.Update(0.016f);
    TEST("exit fires after remove",ex==1);
    TEST("not colliding after remove",!a->IsColliding());

    SECTION("Node hierarchy");
    Node root("root");
    auto* c1=root.AddChild<Node>("c1");
    auto* gc=c1->AddChild<Node>("gc");
    gc->AddChild<Node>("gt");
    TEST("child parent",c1->parent==&root);
    TEST("GetNode deep",root.GetNode("gt")!=nullptr);
    int cnt=0;root.ForEachDescendant([&](Node*){cnt++;});
    TEST("ForEach 3 nodes",cnt==3);
    c1->RemoveChild("gc");
    cnt=0;root.ForEachDescendant([&](Node*){cnt++;});
    TEST("after remove: 1",cnt==1);
}

void test_stress(){
    SECTION("20-object stress");
    const int N=20;
    std::vector<std::unique_ptr<Collider2D>> cols;
    for(int i=0;i<N;i++){
        auto c=std::make_unique<Collider2D>("s"+std::to_string(i));
        c->x=0;c->y=0;c->width=50;c->height=50;c->originX=0;c->originY=0;
        cols.push_back(std::move(c));
    }
    int te=0;
    for(auto&c:cols)c->Connect("on_collision_enter",[&](Collider2D*){te++;});
    CollisionWorld world;
    for(auto&c:cols)world.Add(c.get());
    world.Update();
    TEST("all pairs enter",te==N*(N-1)/2*2);
    TEST("contacts count",cols[0]->GetContacts().size()==(size_t)(N-1));
    int tx=0;
    for(auto&c:cols)c->Connect("on_collision_exit",[&](Collider2D*){tx++;});
    for(int i=0;i<N;i++)cols[i]->x=(float)(i*200);
    world.Update();
    TEST("all exits",tx==N*(N-1)/2*2);
    TEST("contacts cleared",cols[0]->GetContacts().empty());
}

void test_physics(){
    SECTION("Physics");
    const float W=800,H=600,R=20,DT=1.f/60.f;
    Vector2 pos(400,300),vel(237,173);
    int bounces=0;bool px=false,py=false;
    for(int f=0;f<600;f++){
        pos+=vel*DT;
        bool hx=false,hy=false;
        if(pos.x-R<0){pos.x=R;vel.x=std::fabs(vel.x);hx=true;}
        if(pos.x+R>W){pos.x=W-R;vel.x=-std::fabs(vel.x);hx=true;}
        if(pos.y-R<0){pos.y=R;vel.y=std::fabs(vel.y);hy=true;}
        if(pos.y+R>H){pos.y=H-R;vel.y=-std::fabs(vel.y);hy=true;}
        if((hx&&!px)||(hy&&!py))bounces++;
        px=hx;py=hy;
    }
    TEST("in box",pos.x-R>=-0.01f&&pos.x+R<=W+0.01f&&pos.y-R>=-0.01f&&pos.y+R<=H+0.01f);
    TEST("bounces",bounces>0);
    TEST("speed kept",near(vel.Length(),Vector2(237,173).Length()));
}

void test_curves(){
    SECTION("Curves + AnimationPlayer");
    for(auto[nm,e]:std::vector<std::pair<std::string,Ease>>{
        {"Linear",Ease::Linear},{"EaseIn",Ease::EaseIn},{"EaseOut",Ease::EaseOut},
        {"EaseInCubic",Ease::EaseInCubic},{"EaseOutElastic",Ease::EaseOutElastic},
        {"EaseOutBounce",Ease::EaseOutBounce},{"EaseInBack",Ease::EaseInBack},
    }){
        TEST(nm+"(0)==0",near(Curves::Apply(e,0),0));
        TEST(nm+"(1)==1",near(Curves::Apply(e,1),1));
    }
    TEST("EaseIn slow",Curves::Apply(Ease::EaseIn,0.5f)<0.5f);
    TEST("EaseOut fast",Curves::Apply(Ease::EaseOut,0.5f)>0.5f);
    KeyframeTrack t;t.AddKey(0,0,Ease::Linear);t.AddKey(1,100,Ease::Linear);
    TEST("kf t=0.5",near(t.Sample(0.5f),50));
    AnimationPlayer ap("ap");
    Animation jump("jump",false);
    jump.AddFrame(0,0,32,32,.1f);jump.AddFrame(32,0,32,32,.1f);
    jump.displayW=32;jump.displayH=32;jump.displayScale=1;
    ap.Add(jump);ap.Play("jump");
    TEST("playing",ap.IsPlaying());
    ap.Update(sumDur(jump)+0.1f);
    TEST("non-loop finishes",ap.IsFinished());
}

// ─────────────────────────────────────────────────────────────────────────────
// VISUAL TESTS — 1-4=scene, F1=debug, Q=quit
// ─────────────────────────────────────────────────────────────────────────────
static void lbl(const std::string& t,float x,float y,Color c=WHITE){DrawText(t.c_str(),x,y,c);}

// ── Scene 1: Collision signals + IsColliding / GetContacts ───────────────────
static bool sc1_init=false;
static void sc1_setup(Scene& s){
    if(sc1_init)return;sc1_init=true;
    auto* p=s.Add<Collider2D>("player");
    p->shape=ColliderShape::Circle;p->radius=22;p->x=450;p->y=300;
    p->debugDraw=true;p->debugColor=BLUE;
    p->Connect("on_collision_enter",[](Collider2D*){});
    int wi=0;
    for(auto[wx,wy,ww,wh]:std::vector<std::array<float,4>>{
        {120,100,220,22},{600,140,22,180},{180,420,240,22},
        {110,250,22,130},{530,390,180,22},{370,70,22,200}}){
        auto* wall=s.Add<Collider2D>("wall"+std::to_string(wi++));
        wall->shape=ColliderShape::Rectangle;
        wall->x=wx;wall->y=wy;wall->width=ww;wall->height=wh;
        wall->originX=0;wall->originY=0;
        wall->debugDraw=true;wall->debugColor={.55f,.55f,.55f,1};
    }
}
static void sc1_run(Scene& s,float dt){
    sc1_setup(s);
    std::string contacts="(none)";
    bool hit=false;
    if(auto* p=dynamic_cast<Collider2D*>(s.GetNode("player"))){
        float sp=200.f;
        if(IsKeyDown(Key::W))p->y-=sp*dt; if(IsKeyDown(Key::S))p->y+=sp*dt;
        if(IsKeyDown(Key::A))p->x-=sp*dt; if(IsKeyDown(Key::D))p->x+=sp*dt;
        hit=p->IsColliding();
        contacts="";
        for(auto* c:p->GetContacts()) contacts+=c->name+" ";
        if(contacts.empty()) contacts="(none)";
    }
    s.Update(dt);
    s.Draw();

    DrawRectangle(8,8,460,90,{0,0,0,.75f});
    lbl("Scene 1: Collision signals + contacts",14,12);
    lbl("WASD: move blue circle into grey walls",14,32);
    lbl(hit?"COLLIDING: "+contacts:"Not colliding",14,52,hit?RED:GREEN);
    lbl("Hitboxes glow white when touching",14,72);
}

// ── Scene 2: Parent rotation — arms orbit hub using world-space positioning ──
// Arms are SCENE-LEVEL nodes; we manually update their world positions each frame.
// This ensures collision AND debug outlines both use correct world coords.
static bool sc2_init=false;
static float sc2_angle=0;
static const float SC2_OFFSETS[4][2]={{110,0},{0,110},{-110,0},{0,-110}};
static const Color SC2_COLS[4]={RED,GREEN,BLUE,YELLOW};
static const char* SC2_NAMES[4]={"arm0","arm1","arm2","arm3"};

static void sc2_setup(Scene& s){
    if(sc2_init)return;sc2_init=true;
    // Hub: just a position marker
    auto* hub=s.Add<Node2D>("hub"); hub->x=450;hub->y=300;
    // Arms: scene-level so collision world registers them, positions updated each frame
    for(int i=0;i<4;i++){
        auto* arm=s.Add<Collider2D>(SC2_NAMES[i]);
        arm->shape=ColliderShape::Circle;arm->radius=18;
        arm->debugDraw=true;arm->debugColor=SC2_COLS[i];
    }
    // Center marker
    auto* center=s.Add<Collider2D>("center");
    center->shape=ColliderShape::Circle;center->radius=8;
    center->x=450;center->y=300;
    center->debugDraw=true;center->debugColor=WHITE;
}
static void sc2_run(Scene& s,float dt){
    sc2_setup(s);
    sc2_angle+=50.f*dt;
    float rad=sc2_angle*3.14159265f/180.f;
    float cr=std::cos(rad),sr=std::sin(rad);
    // Update each arm to its world position (hub at 450,300 + rotated offset)
    for(int i=0;i<4;i++){
        if(auto* arm=dynamic_cast<Collider2D*>(s.GetNode(SC2_NAMES[i]))){
            float lx=SC2_OFFSETS[i][0],ly=SC2_OFFSETS[i][1];
            arm->x=450+lx*cr-ly*sr;
            arm->y=300+lx*sr+ly*cr;
        }
    }
    s.Update(dt);
    // Orbit ring
    for(int i=0;i<64;i++){
        float a1=i/64.f*6.2832f,a2=(i+1)/64.f*6.2832f;
        DrawLine(450+std::cos(a1)*110,300+std::sin(a1)*110,
                 450+std::cos(a2)*110,300+std::sin(a2)*110,{.2f,.2f,.2f,1});
    }
    // Spoke lines from hub to arms
    for(int i=0;i<4;i++){
        if(auto* arm=dynamic_cast<Collider2D*>(s.GetNode(SC2_NAMES[i]))){
            DrawLine(450,300,arm->x,arm->y,{.3f,.3f,.3f,1});
        }
    }
    s.Draw();

    int desc=0; // count collisions between arms
    int armCollisions=0;
    for(int i=0;i<4;i++){
        auto* arm=dynamic_cast<Collider2D*>(s.GetNode(SC2_NAMES[i]));
        if(arm&&arm->IsColliding()) armCollisions++;
    }
    DrawRectangle(8,8,480,100,{0,0,0,.75f});
    lbl("Scene 2: Parent rotation — arms orbit hub",14,12);
    lbl("4 arms orbit at 50 deg/s — hitboxes follow correctly",14,32);
    lbl("Arm-arm collisions: "+std::to_string(armCollisions/2),14,52);
    lbl("R: toggle spin direction",14,72);
    if(IsKeyPressed(Key::R)) sc2_angle=-sc2_angle; // crude toggle
}

// ── Scene 3: Camera follow + dynamic spawn ───────────────────────────────────
static bool sc3_init=false;
static Camera2D sc3_cam(450,300,1,0);
static int sc3_id=0;
static void sc3_setup(Scene& s){
    if(sc3_init)return;sc3_init=true;
    for(int i=0;i<6;i++){
        auto* o=s.Add<Collider2D>("obs"+std::to_string(i));
        o->shape=ColliderShape::Rectangle;
        o->x=80.f+i*140.f;o->y=180.f+(i%2)*160.f;
        o->width=60;o->height=60;o->originX=.5f;o->originY=.5f;
        o->debugDraw=true;o->debugColor={.2f+i*.1f,.6f,.3f,1};
    }
    auto* p=s.Add<Collider2D>("player3");
    p->shape=ColliderShape::Circle;p->radius=20;p->x=450;p->y=300;
    p->debugDraw=true;p->debugColor=RED;
}
static void sc3_run(Scene& s,float dt){
    sc3_setup(s);
    bool hit=false;
    std::string cs="(none)";
    if(auto* p=dynamic_cast<Collider2D*>(s.GetNode("player3"))){
        float sp=220.f;
        if(IsKeyDown(Key::W))p->y-=sp*dt; if(IsKeyDown(Key::S))p->y+=sp*dt;
        if(IsKeyDown(Key::A))p->x-=sp*dt; if(IsKeyDown(Key::D))p->x+=sp*dt;
        sc3_cam.x+=(p->x-sc3_cam.x)*5.f*dt;
        sc3_cam.y+=(p->y-sc3_cam.y)*5.f*dt;
        hit=p->IsColliding();
        cs="";
        for(auto* c:p->GetContacts()) cs+=c->name+" ";
        if(cs.empty()) cs="(none)";
        if(IsKeyPressed(Key::Space)){
            auto* n=s.Add<Collider2D>("dyn"+std::to_string(sc3_id++));
            n->shape=ColliderShape::Rectangle;
            n->x=p->x+(sc3_id%3-1)*170.f;n->y=p->y+(sc3_id%2)*100.f-50.f;
            n->width=50;n->height=50;n->originX=.5f;n->originY=.5f;
            n->debugDraw=true;n->debugColor={1,.5f,0,1};
        }
    }
    s.Update(dt);
    BeginCamera2D(sc3_cam);
    s.Draw();
    DrawLine(-20,0,20,0,{.35f,.35f,.35f,1});
    DrawLine(0,-20,0,20,{.35f,.35f,.35f,1});
    EndCamera2D();

    DrawRectangle(8,8,490,90,{0,0,0,.75f});
    lbl("Scene 3: Camera follow + spawn",14,12);
    lbl("WASD:move  Space:spawn orange box",14,32);
    lbl(hit?"COLLIDING: "+cs:"Not colliding",14,52,hit?RED:GREEN);
    lbl("Cam:("+std::to_string((int)sc3_cam.x)+","+std::to_string((int)sc3_cam.y)+")",14,72);
}

// ── Scene 4: Layer/mask + 5×5 grid, camera ───────────────────────────────────
static bool sc4_init=false;
static Camera2D sc4_cam(375,300,1,0);
static float sc4_zoom=1,sc4_rot=0;
static int sc4_eHits=0,sc4_wHits=0;
static void sc4_setup(Scene& s){
    if(sc4_init)return;sc4_init=true;
    for(int row=0;row<5;row++) for(int col=0;col<5;col++){
        auto* c=s.Add<Collider2D>("tile"+std::to_string(row*5+col));
        c->shape=ColliderShape::Rectangle;
        c->x=(float)(col*150+75);c->y=(float)(row*120+60);
        c->width=80;c->height=60;c->originX=.5f;c->originY=.5f;
        c->layer=4;c->mask=1;
        c->debugDraw=true;
        c->debugColor={.2f+(float)col*.12f,.2f+(float)row*.12f,.8f,1};
    }
    float ex[]={150,600,375};float ey[]={120,400,540};
    for(int i=0;i<3;i++){
        auto* e=s.Add<Collider2D>("enemy"+std::to_string(i));
        e->shape=ColliderShape::Circle;e->radius=20;
        e->x=ex[i];e->y=ey[i];e->layer=2;e->mask=1;
        e->debugDraw=true;e->debugColor=GREEN;
    }
    auto* p=s.Add<Collider2D>("player4");
    p->shape=ColliderShape::Circle;p->radius=22;p->x=375;p->y=300;
    p->layer=1;p->mask=2|4;
    p->debugDraw=true;p->debugColor=RED;
    p->Connect("on_collision_enter",[](Collider2D* o){
        if(o->name.find("enemy")!=std::string::npos) sc4_eHits++;
        if(o->name.find("tile") !=std::string::npos) sc4_wHits++;
    });
}
static void sc4_run(Scene& s,float dt){
    sc4_setup(s);
    bool hit=false; std::string cs="(none)";
    if(auto* p=dynamic_cast<Collider2D*>(s.GetNode("player4"))){
        float sp=200.f;
        if(IsKeyDown(Key::W))p->y-=sp*dt; if(IsKeyDown(Key::S))p->y+=sp*dt;
        if(IsKeyDown(Key::A))p->x-=sp*dt; if(IsKeyDown(Key::D))p->x+=sp*dt;
        sc4_cam.x+=(p->x-sc4_cam.x)*4.f*dt;
        sc4_cam.y+=(p->y-sc4_cam.y)*4.f*dt;
        sc4_zoom=std::max(.2f,sc4_zoom+GetMouseScroll()*.08f);
        if(IsKeyDown(Key::E))sc4_rot+=30*dt;
        sc4_cam.zoom=sc4_zoom;sc4_cam.rotation=sc4_rot;
        hit=p->IsColliding();
        cs="";
        for(auto* c:p->GetContacts()) cs+=c->name+" ";
        if(cs.empty()) cs="(none)";
    }
    s.Update(dt);
    BeginCamera2D(sc4_cam);
    s.Draw();
    DrawCircle(0,0,6,{.5f,.5f,.5f,1});
    DrawLine(-30,0,30,0,{.35f,.35f,.35f,1});
    DrawLine(0,-30,0,30,{.35f,.35f,.35f,1});
    EndCamera2D();

    DrawRectangle(8,8,540,120,{0,0,0,.75f});
    lbl("Scene 4: Layer/mask + 5x5 grid (camera follows)",14,12);
    lbl("WASD:move  Scroll:zoom  E:rotate",14,32);
    lbl("Red(1,mask=2|4) hits green enemies + blue grid",14,52);
    lbl("Green enemies(2) ignore grid tiles(4)",14,72);
    lbl(hit?"COLLIDING: "+cs:"Not colliding",14,92,hit?RED:GREEN);
    lbl("Enemy:"+std::to_string(sc4_eHits)+" Tile:"+std::to_string(sc4_wHits),14,110);
}

// ── Main visual loop ──────────────────────────────────────────────────────────
void run_visual_tests(){
    std::cout<<"\n========================================\n"
               "  VISUAL TESTS\n"
               "  1-4=scene   F1=debug   Q=quit\n"
               "  Hitboxes glow white when colliding\n"
               "========================================\n";

    InitWindow(900,600,"KonEngine Tests  [1-4=scene  F1=debug  Q=quit]",true);
    // SetTargetFPS(60);
    DebugMode(true); // start with debug on so outlines are always visible

    Scene s1,s2,s3,s4;
    int cur=1;
    const char* labels[]={"","1:Signals","2:Orbit","3:CameraSpawn","4:LayerGrid"};

    while(!WindowShouldClose()){
        float dt=GetDeltaTime();
        if(IsKeyPressed(Key::F1))  DebugMode(!IsDebugMode());
        if(IsKeyPressed(Key::Num1))cur=1;
        if(IsKeyPressed(Key::Num2))cur=2;
        if(IsKeyPressed(Key::Num3))cur=3;
        if(IsKeyPressed(Key::Num4))cur=4;
        if(IsKeyPressed(Key::Q))   break;

        ClearBackground(.07f,.07f,.10f);
        switch(cur){
            case 1:sc1_run(s1,dt);break;
            case 2:sc2_run(s2,dt);break;
            case 3:sc3_run(s3,dt);break;
            case 4:sc4_run(s4,dt);break;
        }
        // Scene label top-right
        int W=GetWindowWidth();
        DrawRectangle(W-185,8,177,40,{0,0,0,.75f});
        DrawText(labels[cur],W-181,12,WHITE);
        DrawText(IsDebugMode()?"[F1] DEBUG ON":"[F1] debug off",W-181,28,
                 IsDebugMode()?YELLOW:Color{.4f,.4f,.4f,1});
        Present();PollEvents();
    }
    std::cout<<"  Visual tests complete.\n";
}

// ─────────────────────────────────────────────────────────────────────────────
int main(){
    std::cout<<"========================================\n"
               "  KonEngine Test Suite\n"
               "========================================\n";
    test_vector2(); test_primitives(); test_sat();
    test_layer_mask(); test_contacts(); test_signals();
    test_scene(); test_stress(); test_physics(); test_curves();
    std::cout<<"\n========================================\n"
             <<"  Headless: "<<s_pass<<" passed, "<<s_fail<<" failed\n"
               "========================================\n";
    if(s_fail>0)std::cout<<"  Some tests failed!\n";
    else        std::cout<<"  All headless tests passed!\n";
    run_visual_tests();
    return s_fail>0?1:0;
}
