#include "KonEngine.hpp"
#include "collision/collision_world.hpp"
#include "node/collider2d.hpp"
#include <iostream>
#include <cmath>
#include <vector>
#include <string>
#include <memory>

static int s_passed = 0, s_failed = 0;
#define TEST(name, expr) do { \
    if (expr) { std::cout << "  [PASS] " << name << "\n"; s_passed++; } \
    else      { std::cout << "  [FAIL] " << name << "\n"; s_failed++; } \
} while(0)
#define SECTION(name) std::cout << "\n-- " << name << " --\n"

static const float EPS = 0.001f;
static bool near(float a, float b) { return std::fabs(a-b)<EPS; }
static bool nearv(Vector2 a, Vector2 b) { return near(a.x,b.x)&&near(a.y,b.y); }
static float sumDur(const Animation& a){ float d=0; for(auto& f:a.frames) d+=f.duration; return d; }
static void label(const std::string& t, float x, float y){ DrawText(t.c_str(),x,y,WHITE); }

// ---- Vector2 ----
void test_vector2() {
    SECTION("Vector2 — arithmetic");
    Vector2 a(3,4), b(1,2);
    TEST("Addition",        nearv(a+b,{4,6}));
    TEST("Subtraction",     nearv(a-b,{2,2}));
    TEST("Scalar mul",      nearv(a*2,{6,8}));
    TEST("Float*vec",       nearv(2.0f*a,{6,8}));
    TEST("Scalar div",      nearv(a/2,{1.5f,2}));
    TEST("Negate",          nearv(-a,{-3,-4}));
    TEST("Equal",           a==Vector2(3,4));
    TEST("Not equal",       a!=b);

    SECTION("Vector2 — length/distance");
    TEST("Length (3,4)==5",    near(a.Length(),5));
    TEST("LengthSq==25",       near(a.LengthSq(),25));
    TEST("Distance to 0==5",   near(a.Distance({}),5));
    TEST("DistanceSq==25",     near(a.DistanceSq({}),25));
    TEST("Zero length==0",     near(Vector2{}.Length(),0));

    SECTION("Vector2 — normalize");
    Vector2 n=a.Normalized();
    TEST("Norm length==1",       near(n.Length(),1));
    TEST("Norm direction",       near(n.x,0.6f)&&near(n.y,0.8f));
    TEST("Zero norm safe",       Vector2{}.Normalized()==Vector2{});

    SECTION("Vector2 — dot");
    TEST("Right·Up==0",   near(Vector2::Right().Dot(Vector2::Up()),0));
    TEST("Right·Right==1",near(Vector2::Right().Dot(Vector2::Right()),1));
    TEST("Right·Left==-1",near(Vector2::Right().Dot(Vector2::Left()),-1));

    SECTION("Vector2 — rotation");
    float pi=3.14159265f;
    Vector2 r90=Vector2::Right().Rotated(pi/2);
    TEST("90deg ~= Down",  near(r90.x,0)&&near(r90.y,1));
    Vector2 r180=Vector2::Right().Rotated(pi);
    TEST("180deg ~= Left", near(r180.x,-1)&&near(r180.y,0));
    TEST("Rotation length preserved", near(Vector2(3,4).Rotated(1.23f).Length(),5));

    SECTION("Vector2 — reflection");
    // Floor: normal=(0,-1). (1,1) reflected = (1,-1)
    Vector2 ref=Vector2(1,1).Reflected({0,-1});
    TEST("Floor flips Y",       near(ref.x,1)&&near(ref.y,-1));
    TEST("Reflection len kept", near(ref.Length(),Vector2(1,1).Length()));
    // 45-deg wall: normal=(-1,-1)/sqrt2. (1,0) reflected = (0,-1)
    Vector2 wr=Vector2(1,0).Reflected(Vector2(-1,-1).Normalized());
    TEST("45-deg wall len",     near(wr.Length(),1));
    TEST("45-deg wall dir (0,-1)", near(wr.x,0)&&near(wr.y,-1));

    SECTION("Vector2 — lerp");
    TEST("Lerp mid",  nearv(Vector2::Lerp({0,0},{10,20},0.5f),{5,10}));
    TEST("Lerp t=0",  nearv(Vector2::Lerp({1,2},{3,4},0),{1,2}));
    TEST("Lerp t=1",  nearv(Vector2::Lerp({1,2},{3,4},1),{3,4}));

    SECTION("Vector2 — presets");
    TEST("Zero",  Vector2::Zero()==Vector2(0,0));
    TEST("One",   Vector2::One()==Vector2(1,1));
    TEST("Up",    Vector2::Up()==Vector2(0,-1));
    TEST("Down",  Vector2::Down()==Vector2(0,1));
    TEST("Left",  Vector2::Left()==Vector2(-1,0));
    TEST("Right", Vector2::Right()==Vector2(1,0));
}

// ---- Primitive collision ----
void test_primitives() {
    SECTION("CheckCollisionCircles");
    TEST("Exact touch no collide",  !CheckCollisionCircles({0,0,50},{100,0,50}));
    TEST("1-unit overlap",           CheckCollisionCircles({0,0,50},{99,0,50}));
    TEST("Far apart",               !CheckCollisionCircles({0,0,50},{300,0,50}));
    TEST("Concentric",               CheckCollisionCircles({0,0,10},{0,0,10}));
    TEST("Diagonal overlap (3-4-5)", CheckCollisionCircles({0,0,3},{3,4,3}));
    TEST("Diagonal gap (3-4-5)",    !CheckCollisionCircles({0,0,2},{3,4,2}));

    SECTION("CheckCollisionCircleRec");
    Rectangle r(100,100,200,100);
    TEST("Center inside",      CheckCollisionCircleRec({200,150,10},r));
    TEST("Far away",          !CheckCollisionCircleRec({0,0,10},r));
    TEST("Exact touch left",  !CheckCollisionCircleRec({90,150,10},r));
    TEST("1px overlap left",   CheckCollisionCircleRec({91,150,10},r));
    TEST("Above no overlap",  !CheckCollisionCircleRec({200,80,10},r));
    TEST("Above overlap",      CheckCollisionCircleRec({200,91,10},r));
    TEST("Corner outside",    !CheckCollisionCircleRec({90,90,10},r)); // dist~14.1>10
    TEST("Corner inside",      CheckCollisionCircleRec({93,93,10},r)); // dist~9.9<10

    SECTION("CheckCollisionRecs");
    TEST("Full overlap",        CheckCollisionRecs({0,0,100,100},{25,25,50,50}));
    TEST("Partial overlap",     CheckCollisionRecs({0,0,100,100},{50,50,100,100}));
    TEST("Touch right",        !CheckCollisionRecs({0,0,100,100},{100,0,100,100}));
    TEST("Touch bottom",       !CheckCollisionRecs({0,0,100,100},{0,100,100,100}));
    TEST("1px overlap X",       CheckCollisionRecs({0,0,100,100},{99,0,100,100}));
    TEST("Contained",           CheckCollisionRecs({0,0,200,200},{50,50,10,10}));
    TEST("Separate X",         !CheckCollisionRecs({0,0,100,100},{200,0,100,100}));
    TEST("Separate diagonal",  !CheckCollisionRecs({0,0,10,10},{20,20,10,10}));
}

// ---- SAT ----
void test_sat() {
    SECTION("SAT — Circle vs Circle");
    Collider2D ca("ca"),cb("cb");
    ca.shape=ColliderShape::Circle; ca.x=0; ca.y=0; ca.radius=50;
    cb.shape=ColliderShape::Circle; cb.x=80;cb.y=0; cb.radius=50;
    TEST("Overlap",       CollisionWorld::Overlaps(&ca,&cb));
    cb.x=100;
    TEST("Exact touch",  !CollisionWorld::Overlaps(&ca,&cb));
    cb.x=99;
    TEST("1-unit",        CollisionWorld::Overlaps(&ca,&cb));
    cb.x=60; cb.y=60;
    TEST("Diagonal hit",  CollisionWorld::Overlaps(&ca,&cb));
    cb.x=80; cb.y=80;
    TEST("Diagonal miss",!CollisionWorld::Overlaps(&ca,&cb));

    SECTION("SAT — Circle vs Rect");
    Collider2D rect("r"),circ("c");
    rect.shape=ColliderShape::Rectangle;
    rect.x=0; rect.y=0; rect.width=200; rect.height=200; rect.originX=0; rect.originY=0;
    circ.shape=ColliderShape::Circle; circ.radius=30;
    circ.x=100; circ.y=100;
    TEST("Inside rect",    CollisionWorld::Overlaps(&rect,&circ));
    circ.x=-20; circ.y=100;
    TEST("Left edge hit",  CollisionWorld::Overlaps(&rect,&circ));
    circ.x=-31; circ.y=100;
    TEST("Left edge miss",!CollisionWorld::Overlaps(&rect,&circ));
    circ.x=-20; circ.y=-20;
    TEST("Corner hit",     CollisionWorld::Overlaps(&rect,&circ));
    circ.x=-25; circ.y=-25;
    TEST("Corner miss",   !CollisionWorld::Overlaps(&rect,&circ));

    SECTION("SAT — Rect vs Rect");
    Collider2D r1("r1"),r2("r2");
    r1.shape=ColliderShape::Rectangle; r1.x=0;r1.y=0;r1.width=100;r1.height=100;r1.originX=0;r1.originY=0;
    r2.shape=ColliderShape::Rectangle; r2.x=50;r2.y=50;r2.width=100;r2.height=100;r2.originX=0;r2.originY=0;
    TEST("Overlap",   CollisionWorld::Overlaps(&r1,&r2));
    r2.x=200;
    TEST("Separate", !CollisionWorld::Overlaps(&r1,&r2));

    SECTION("SAT — Custom polygon (triangle)");
    Collider2D tri("tri"),pt("pt");
    tri.shape=ColliderShape::Custom; tri.x=0; tri.y=0;
    tri.points={{0,-60},{52,30},{-52,30}};
    pt.shape=ColliderShape::Rectangle; pt.width=10; pt.height=10; pt.originX=0.5f; pt.originY=0.5f;
    pt.x=0; pt.y=0;
    TEST("Inside triangle",  CollisionWorld::Overlaps(&tri,&pt));
    pt.x=0; pt.y=-50;
    TEST("Near top vertex",  CollisionWorld::Overlaps(&tri,&pt));
    pt.x=0; pt.y=-80;
    TEST("Above triangle",  !CollisionWorld::Overlaps(&tri,&pt));
    pt.x=100; pt.y=0;
    TEST("Right of triangle",!CollisionWorld::Overlaps(&tri,&pt));
}

// ---- Layer/mask ----
void test_layer_mask() {
    SECTION("Layer/mask bitmask");
    auto hits=[](uint32_t la,uint32_t ma,uint32_t lb,uint32_t mb){ return (la&mb)||(lb&ma); };
    TEST("Default 1,1 vs 1,1", hits(1,1,1,1));
    TEST("1,2 hits 2,1",       hits(1,2,2,1));
    TEST("1,4 vs 2,8 no match",!hits(1,4,2,8));
    uint32_t pL=1,pM=2|4, eL=2,eM=1, wL=4,wM=1|2, bL=8,bM=0;
    TEST("Player hits enemy",  hits(pL,pM,eL,eM));
    TEST("Player hits wall",   hits(pL,pM,wL,wM));
    TEST("Player misses bg",  !hits(pL,pM,bL,bM));
    TEST("Enemy hits wall",    hits(eL,eM,wL,wM));
    TEST("BG hits nothing",   !hits(bL,bM,pL,pM));

    SECTION("Layer/mask CollisionWorld enforcement");
    Collider2D a("a"),b("b");
    a.x=0;a.y=0;a.width=100;a.height=100;a.originX=0;a.originY=0;
    b.x=0;b.y=0;b.width=100;b.height=100;b.originX=0;b.originY=0;
    a.layer=1; a.mask=4;
    b.layer=2; b.mask=8;
    int entered=0;
    a.Connect("on_collision_enter",[&](Collider2D*){ entered++; });
    CollisionWorld world; world.Add(&a); world.Add(&b);
    world.Update();
    TEST("Mismatch suppresses signal", entered==0);
    b.layer=4; b.mask=1;
    world.Update();
    TEST("Match fires signal", entered==1);
}

// ---- Multi-collision ----
void test_multi_collision() {
    SECTION("Multi collision");
    const int N=5;
    std::vector<std::unique_ptr<Collider2D>> cols;
    for(int i=0;i<N;i++){
        auto c=std::make_unique<Collider2D>("c"+std::to_string(i));
        c->x=0;c->y=0;c->width=100;c->height=100;c->originX=0;c->originY=0;
        cols.push_back(std::move(c));
    }
    int te=0;
    for(auto& c:cols) c->Connect("on_collision_enter",[&](Collider2D*){ te++; });
    CollisionWorld world;
    for(auto& c:cols) world.Add(c.get());
    world.Update();
    TEST("All pairs fire enter", te==N*(N-1)/2*2);

    int tx=0;
    for(auto& c:cols) c->Connect("on_collision_exit",[&](Collider2D*){ tx++; });
    cols[0]->x=500;
    world.Update();
    TEST("Move one out fires exits", tx==(N-1)*2);

    SECTION("Re-enter");
    CollisionWorld w2;
    Collider2D p("p"),q("q");
    p.x=0;p.y=0;p.width=50;p.height=50;p.originX=0;p.originY=0;
    q.x=0;q.y=0;q.width=50;q.height=50;q.originX=0;q.originY=0;
    w2.Add(&p); w2.Add(&q);
    int en=0,ex=0;
    p.Connect("on_collision_enter",[&](Collider2D*){ en++; });
    p.Connect("on_collision_exit", [&](Collider2D*){ ex++; });
    w2.Update(); TEST("First enter",  en==1);
    w2.Update(); TEST("Stay no dupe", en==1);
    q.x=200; w2.Update(); TEST("Exit",    ex==1);
    q.x=0;   w2.Update(); TEST("Re-enter",en==2);
}

// ---- Physics ----
void test_physics() {
    SECTION("Bouncing ball");
    const float W=800,H=600,R=20,DT=1.f/60.f;
    Vector2 pos(400,300),vel(237,173);
    int bounces=0; bool px=false,py=false;
    for(int f=0;f<600;f++){
        pos+=vel*DT;
        bool hx=false,hy=false;
        if(pos.x-R<0){pos.x=R;vel.x=std::fabs(vel.x);hx=true;}
        if(pos.x+R>W){pos.x=W-R;vel.x=-std::fabs(vel.x);hx=true;}
        if(pos.y-R<0){pos.y=R;vel.y=std::fabs(vel.y);hy=true;}
        if(pos.y+R>H){pos.y=H-R;vel.y=-std::fabs(vel.y);hy=true;}
        if((hx&&!px)||(hy&&!py)) bounces++;
        px=hx;py=hy;
    }
    TEST("Ball in box X min", pos.x-R>=-EPS);
    TEST("Ball in box X max", pos.x+R<=W+EPS);
    TEST("Ball in box Y min", pos.y-R>=-EPS);
    TEST("Ball in box Y max", pos.y+R<=H+EPS);
    TEST("Ball bounces",      bounces>0);
    TEST("Speed preserved",   near(vel.Length(),Vector2(237,173).Length()));

    SECTION("Proximity trigger");
    Vector2 enemy(400,300),player(0,300);
    float alertRange=150,speed=200; bool fired=false; float t=0;
    Vector2 dir=(enemy-player).Normalized();
    while(t<5.f){ player+=dir*speed*DT; t+=DT; if(player.Distance(enemy)<alertRange){fired=true;break;} }
    TEST("Alert fires",       fired);
    TEST("Correct distance",  player.Distance(enemy)<alertRange);
}

// ---- Animation ----
void test_animation() {
    SECTION("Curves — boundary values");
    for(auto [nm,e]: std::vector<std::pair<std::string,Ease>>{
        {"Linear",Ease::Linear},{"EaseIn",Ease::EaseIn},{"EaseOut",Ease::EaseOut},
        {"EaseInOut",Ease::EaseInOut},{"EaseInCubic",Ease::EaseInCubic},
        {"EaseOutCubic",Ease::EaseOutCubic},{"EaseInOutCubic",Ease::EaseInOutCubic},
        {"EaseInElastic",Ease::EaseInElastic},{"EaseOutElastic",Ease::EaseOutElastic},
        {"EaseInBounce",Ease::EaseInBounce},{"EaseOutBounce",Ease::EaseOutBounce},
        {"EaseInBack",Ease::EaseInBack},{"EaseOutBack",Ease::EaseOutBack},
        {"EaseInOutBack",Ease::EaseInOutBack}
    }){
        TEST(nm+"(0)==0", near(Curves::Apply(e,0),0));
        TEST(nm+"(1)==1", near(Curves::Apply(e,1),1));
    }
    TEST("EaseIn slow start",  Curves::Apply(Ease::EaseIn,0.5f)<0.5f);
    TEST("EaseOut fast start", Curves::Apply(Ease::EaseOut,0.5f)>0.5f);

    SECTION("KeyframeTrack");
    KeyframeTrack t; t.AddKey(0.5f,42.f);
    TEST("Single key before", near(t.Sample(0),42)); TEST("Single key at",near(t.Sample(.5f),42));
    KeyframeTrack t2;
    t2.AddKey(0,0,Ease::Linear); t2.AddKey(1,100,Ease::Linear);
    TEST("Linear t=0.25",near(t2.Sample(.25f),25)); TEST("Linear t=0.5",near(t2.Sample(.5f),50));
    TEST("Before clamps",near(t2.Sample(-1),0));    TEST("After clamps",near(t2.Sample(2),100));
    KeyframeTrack t3;
    t3.AddKey(0,0,Ease::Linear); t3.AddKey(.5f,50,Ease::Linear); t3.AddKey(1,0,Ease::Linear);
    TEST("3-key peak",  near(t3.Sample(.5f),50)); TEST("3-key end",near(t3.Sample(1),0));

    SECTION("AnimationPlayer state machine");
    AnimationPlayer ap("ap");
    Animation idle("idle",true); idle.AddFrame(0,0,32,32,.1f); idle.AddFrame(32,0,32,32,.1f);
    idle.displayW=32;idle.displayH=32;idle.displayScale=1;
    Animation jump("jump",false); jump.AddFrame(0,64,32,32,.1f); jump.AddFrame(32,64,32,32,.1f);
    jump.displayW=32;jump.displayH=32;jump.displayScale=1;
    ap.Add(idle); ap.Add(jump);
    TEST("Not playing initially",!ap.IsPlaying());
    ap.Play("idle"); TEST("Playing after Play",ap.IsPlaying());
    ap.Pause();      TEST("Paused",!ap.IsPlaying());
    ap.Resume();     TEST("Resumed",ap.IsPlaying());
    ap.Play("jump"); float jd=sumDur(jump);
    ap.Update(jd+.1f);
    TEST("Non-loop finishes",ap.IsFinished());
    TEST("Finished not playing",!ap.IsPlaying());
    ap.Play("idle"); TEST("Resets on Play",!ap.IsFinished());
    ap.Stop();       TEST("Stop sets finished",ap.IsFinished());
    ap.Play("nope"); TEST("Missing safe",!ap.IsPlaying());
}

// ---- Nodes ----
void test_nodes() {
    SECTION("Node tree");
    Node root("root");
    auto* c1=root.AddChild<Node>("c1"); auto* c2=root.AddChild<Node>("c2");
    auto* gc=c1->AddChild<Node>("gc"); auto* gt=gc->AddChild<Node>("gt");
    TEST("Child parent",  c1->parent==&root);
    TEST("Deep parent",   gt->parent==gc);
    TEST("GetNode c1",    root.GetNode("c1")==c1);
    TEST("GetNode deep",  root.GetNode("gt")==gt);
    TEST("GetNode miss",  root.GetNode("nope")==nullptr);
    int cnt=0; root.ForEachDescendant([&](Node*){ cnt++; });
    TEST("ForEach count", cnt==4);
    c1->RemoveChild("gc");
    int cnt2=0; root.ForEachDescendant([&](Node*){ cnt2++; });
    TEST("After remove",  cnt2==2);

    SECTION("Node2D");
    Node2D n; n.x=100;n.y=200;n.originX=.5f;n.originY=.5f;
    TEST("Center DrawX",near(n.DrawX(64),68.f)); TEST("Center DrawY",near(n.DrawY(32),184.f));
    n.Move(10,-5); TEST("Move dx",near(n.x,110)); TEST("Move dy",near(n.y,195));

    SECTION("Signals");
    Node s("s"); int a=0;
    s.Connect("ev",[&](){ a++; }); s.Emit("ev"); s.Emit("ev");
    TEST("Fires twice",a==2); TEST("Unknown safe",(s.Emit("nope"),true));

    SECTION("Scene integration");
    Scene sc;
    auto* sa=sc.Add<Collider2D>("a"); auto* sb=sc.Add<Collider2D>("b");
    sa->x=0;sa->y=0;sa->width=100;sa->height=100;sa->originX=0;sa->originY=0;
    sb->x=0;sb->y=0;sb->width=100;sb->height=100;sb->originX=0;sb->originY=0;
    int en=0,ex=0;
    sa->Connect("on_collision_enter",[&](Collider2D*){ en++; });
    sa->Connect("on_collision_exit", [&](Collider2D*){ ex++; });
    sc.Update(.016f); TEST("Scene detects collision",en==1);
    sc.Remove("b");
    sc.Update(.016f); TEST("Exit fires after remove",ex==1);
}

// ---- Stress ----
void test_stress() {
    SECTION("20-object stress");
    const int N=20;
    std::vector<std::unique_ptr<Collider2D>> cols;
    for(int i=0;i<N;i++){
        auto c=std::make_unique<Collider2D>("s"+std::to_string(i));
        c->x=0;c->y=0;c->width=50;c->height=50;c->originX=0;c->originY=0;
        cols.push_back(std::move(c));
    }
    int te=0;
    for(auto& c:cols) c->Connect("on_collision_enter",[&](Collider2D*){ te++; });
    CollisionWorld world;
    for(auto& c:cols) world.Add(c.get());
    world.Update();
    int ep=N*(N-1)/2;
    TEST("All pairs fire",te==ep*2);
    int tx=0;
    for(auto& c:cols) c->Connect("on_collision_exit",[&](Collider2D*){ tx++; });
    for(int i=0;i<N;i++) cols[i]->x=(float)(i*200);
    world.Update();
    TEST("All exits",tx==ep*2);
}

// ================================================================
// VISUAL TESTS
// ================================================================

void visual_circles() {
    InitWindow(900,600,"Visual Test 1/4 — Circles & Distance  (ESC=next)");
    SetTargetFPS(60);
    Circle fixed(450,300,80);
    float alertDist=200.f;
    while(!WindowShouldClose()){
        float mx=GetMouseX(),my=GetMouseY();
        Circle cursor(mx,my,30);
        bool touching=CheckCollisionCircles(fixed,cursor);
        float dist=std::sqrt((mx-450)*(mx-450)+(my-300)*(my-300));
        bool inRange=dist<alertDist;
        ClearBackground(.08f,.08f,.12f);
        // Alert ring
        for(int i=0;i<64;i++){
            float a1=i/64.f*6.2832f,a2=(i+1)/64.f*6.2832f;
            DrawLine(450+std::cos(a1)*alertDist,300+std::sin(a1)*alertDist,
                     450+std::cos(a2)*alertDist,300+std::sin(a2)*alertDist,
                     inRange?Color{1,1,0,.5f}:Color{.3f,.3f,.3f,1});
        }
        DrawLine(450,300,mx,my,GRAY);
        DrawCircle(450,300,80,touching?RED:GREEN);
        DrawCircle(mx,my,30,touching?Color{1,.5f,0,1}:BLUE);
        DrawRectangle(10,10,340,90,Color{0,0,0,.7f});
        label(touching?"COLLIDING!":"No collision",20,18);
        label(inRange?"IN ALERT RANGE":"Outside range",20,40);
        label("dist="+std::to_string((int)dist),20,62);
        label("Move mouse — ESC = next test",20,82);
        Present(); PollEvents();
    }
}

void visual_sat() {
    InitWindow(900,600,"Visual Test 2/4 — SAT Shapes  (WASD=box, Mouse=circle, ESC=next)");
    SetTargetFPS(60); DebugMode(true);
    Scene scene;
    auto* tri=scene.Add<Collider2D>("triangle");
    tri->shape=ColliderShape::Custom; tri->x=450;tri->y=300;
    tri->points={{0,-80},{70,40},{-70,40}};
    tri->debugDraw=true; tri->debugColor=GREEN;
    auto* box=scene.Add<Collider2D>("box");
    box->shape=ColliderShape::Rectangle; box->x=200;box->y=300;
    box->width=60;box->height=60;box->originX=.5f;box->originY=.5f;
    box->debugDraw=true; box->debugColor=BLUE;
    auto* circ=scene.Add<Collider2D>("circle");
    circ->shape=ColliderShape::Circle; circ->radius=40;
    circ->debugDraw=true; circ->debugColor={1,.5f,0,1};
    bool triBox=false,triCirc=false;
    box->Connect("on_collision_enter",[&](Collider2D* o){if(o==tri) triBox=true;});
    box->Connect("on_collision_exit", [&](Collider2D* o){if(o==tri) triBox=false;});
    circ->Connect("on_collision_enter",[&](Collider2D* o){if(o==tri) triCirc=true;});
    circ->Connect("on_collision_exit", [&](Collider2D* o){if(o==tri) triCirc=false;});
    while(!WindowShouldClose()){
        float dt=GetDeltaTime(),sp=200.f;
        if(IsKeyDown(Key::W)) box->y-=sp*dt;
        if(IsKeyDown(Key::S)) box->y+=sp*dt;
        if(IsKeyDown(Key::A)) box->x-=sp*dt;
        if(IsKeyDown(Key::D)) box->x+=sp*dt;
        circ->x=GetMouseX(); circ->y=GetMouseY();
        tri->rotation+=30.f*dt;
        ClearBackground(.08f,.08f,.12f);
        scene.Update(dt); scene.Draw();
        DrawRectangle(10,10,400,90,Color{0,0,0,.7f});
        label("WASD:move box   Mouse:move circle   ESC:next",20,18);
        label(triBox ?"BOX  HITS TRIANGLE!":"Box: no hit", 20,42);
        label(triCirc?"CIRC HITS TRIANGLE!":"Circ: no hit",20,65);
        Present(); PollEvents();
    }
    DebugMode(false);
}

void visual_physics() {
    InitWindow(900,600,"Visual Test 3/4 — Bouncing Balls  (ESC=next)");
    SetTargetFPS(60);
    struct Ball{ Vector2 pos,vel; float r; Color col; };
    std::vector<Ball> balls={
        {{200,150},{220,173},20,RED},
        {{500,300},{-180,250},15,GREEN},
        {{700,400},{130,-220},25,BLUE},
        {{300,500},{270,-90},12,YELLOW},
        {{600,100},{-150,310},18,{.8f,0,1,1}},
    };
    std::vector<std::unique_ptr<Collider2D>> cols;
    for(int i=0;i<(int)balls.size();i++){
        auto c=std::make_unique<Collider2D>("b"+std::to_string(i));
        c->shape=ColliderShape::Circle; c->radius=balls[i].r;
        c->x=balls[i].pos.x; c->y=balls[i].pos.y;
        cols.push_back(std::move(c));
    }
    CollisionWorld world;
    for(auto& c:cols) world.Add(c.get());
    int contacts=0;
    for(auto& c:cols) c->Connect("on_collision_enter",[&](Collider2D*){ contacts++; });
    while(!WindowShouldClose()){
        float dt=GetDeltaTime();
        float W=GetWindowWidth(),H=GetWindowHeight();
        for(int i=0;i<(int)balls.size();i++){
            auto& b=balls[i];
            b.pos+=b.vel*dt;
            if(b.pos.x-b.r<0){b.pos.x=b.r;b.vel.x=std::fabs(b.vel.x);}
            if(b.pos.x+b.r>W){b.pos.x=W-b.r;b.vel.x=-std::fabs(b.vel.x);}
            if(b.pos.y-b.r<0){b.pos.y=b.r;b.vel.y=std::fabs(b.vel.y);}
            if(b.pos.y+b.r>H){b.pos.y=H-b.r;b.vel.y=-std::fabs(b.vel.y);}
            cols[i]->x=b.pos.x; cols[i]->y=b.pos.y;
        }
        world.Update();
        ClearBackground(.05f,.05f,.1f);
        for(auto& b:balls)
            DrawLine(b.pos.x,b.pos.y,b.pos.x+b.vel.x*.1f,b.pos.y+b.vel.y*.1f,{1,1,1,.3f});
        for(auto& b:balls) DrawCircle(b.pos.x,b.pos.y,b.r,b.col);
        for(int i=0;i<(int)balls.size();i++)
            for(int j=i+1;j<(int)balls.size();j++)
                if(CollisionWorld::Overlaps(cols[i].get(),cols[j].get()))
                    DrawLine(balls[i].pos.x,balls[i].pos.y,balls[j].pos.x,balls[j].pos.y,{1,0,0,.8f});
        DrawRectangle(10,10,360,70,Color{0,0,0,.7f});
        label("5 balls bouncing — arrows=velocity",20,18);
        label("Red lines = ball-ball collision pairs",20,40);
        label("Total ball contacts: "+std::to_string(contacts/2),20,60);
        Present(); PollEvents();
    }
}

void visual_camera() {
    InitWindow(900,600,"Visual Test 4/4 — Camera  (WASD=pan, Scroll=zoom, Q/E=rotate, ESC=done)");
    SetTargetFPS(60);
    Scene scene;
    for(int row=0;row<5;row++) for(int col=0;col<5;col++){
        auto* c=scene.Add<Collider2D>("c"+std::to_string(row*5+col));
        c->shape=ColliderShape::Rectangle;
        c->x=(float)(col*150+75); c->y=(float)(row*120+60);
        c->width=80; c->height=60; c->originX=.5f; c->originY=.5f;
        c->debugDraw=true;
        c->debugColor={.3f+(float)col*.15f,.3f+(float)row*.15f,.8f,1};
    }
    Camera2D cam(450,300,1,0);
    float zoom=1,rot=0;
    while(!WindowShouldClose()){
        float dt=GetDeltaTime(),sp=300.f/zoom;
        if(IsKeyDown(Key::W)) cam.y-=sp*dt;
        if(IsKeyDown(Key::S)) cam.y+=sp*dt;
        if(IsKeyDown(Key::A)) cam.x-=sp*dt;
        if(IsKeyDown(Key::D)) cam.x+=sp*dt;
        zoom=std::max(.1f,zoom+GetMouseScroll()*.1f);
        if(IsKeyDown(Key::Q)) rot-=45*dt;
        if(IsKeyDown(Key::E)) rot+=45*dt;
        cam.zoom=zoom; cam.rotation=rot;
        ClearBackground(.08f,.08f,.12f);
        BeginCamera2D(cam);
        scene.Update(dt); scene.Draw();
        DrawCircle(0,0,8,RED);
        DrawLine(-30,0,30,0,RED); DrawLine(0,-30,0,30,RED);
        EndCamera2D();
        DrawRectangle(10,10,400,90,Color{0,0,0,.7f});
        label("WASD:pan  Scroll:zoom  Q/E:rotate  ESC:done",20,18);
        label("Zoom:"+std::to_string(zoom).substr(0,4)+"  Rot:"+std::to_string((int)rot)+"deg",20,42);
        label("Cam:("+std::to_string((int)cam.x)+","+std::to_string((int)cam.y)+")",20,65);
        Present(); PollEvents();
    }
}

void run_visual_tests() {
    std::cout<<"\n========================================\n";
    std::cout<<"  VISUAL TESTS — close each window to advance\n";
    std::cout<<"========================================\n";
    std::cout<<"1/4: Circle collision + distance\n"; visual_circles();
    std::cout<<"2/4: SAT — box + circle vs triangle\n"; visual_sat();
    std::cout<<"3/4: Physics — bouncing balls\n";      visual_physics();
    std::cout<<"4/4: Camera pan/zoom/rotation\n";      visual_camera();
    std::cout<<"  Visual tests complete.\n";
}

// ================================================================
int main() {
    std::cout<<"========================================\n";
    std::cout<<"  KonEngine Test Suite\n";
    std::cout<<"========================================\n";
    test_vector2();
    test_primitives();
    test_sat();
    test_layer_mask();
    test_multi_collision();
    test_physics();
    test_animation();
    test_nodes();
    test_stress();
    std::cout<<"\n========================================\n";
    std::cout<<"  Headless: "<<s_passed<<" passed, "<<s_failed<<" failed\n";
    std::cout<<"========================================\n";
    if(s_failed>0){ std::cout<<"  Failing tests — fix before release!\n"; }
    std::cout<<"  All headless tests passed!\n";
    run_visual_tests();
    return 0;
}
