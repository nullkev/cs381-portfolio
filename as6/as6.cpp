//Author: Kevin Tran
//Date: 03.XX.2026
//Purpose: Game with pokemon styled combat, using the characters from Azur Lane.

#include "AudioDevice.hpp"
#include "BoundingBox.hpp"
#include "Camera3D.hpp"
#include "Keyboard.hpp"
#include "Matrix.hpp"
#include "Model.hpp"
#include "RadiansDegrees.hpp"
#include "Text.hpp"
#include "Vector2.hpp"
#include "Vector3.hpp"
#include "Vector4.hpp"
#include "raylib-cpp.hpp"
#include "raylib.h"
#include "raymath.h"
#include <concepts>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <memory>
#include <string>
#include <vector>

class Component{
    public:
        struct Entity* attached;
        virtual void Setup() = 0;
        virtual void Update(float dt) = 0;
        virtual void Cleanup() = 0;
};

struct Entity{
    std::vector<std::shared_ptr<Component>> components;
    template<std::derived_from<Component> T>
    T& AddComponent(){
        auto out = components.emplace_back(std::make_shared<T>());
        out -> attached = this;
        return (T&)*out;
    }
    template<std::derived_from<Component> T>
    std::optional<std::reference_wrapper<T>> GetComponent(){
        for(auto& component: components){
            T* cast = dynamic_cast<T*>(component.get());
            if(cast) return *cast;
        }
        return {};
    }
};

struct TransformComponent: public Component{
    raylib::Vector3 position = {0,0, 0};
    raylib::Quaternion rotation = raylib::Quaternion::Identity();
    void Setup() override{}
    void Update(float dt) override{}
    void Cleanup() override{}
};

struct DrawModelComponent: public Component{
    raylib::Model* model;
    bool turn = false;
    void Setup() override{}
    void Update(float dt) override {
        if(auto t = attached->GetComponent<TransformComponent>(); t){
            auto& transform = t->get();
            raylib::Transform backupTransform = model->transform;
            model->transform = raylib::Transform(model->transform).Translate(transform.position).Rotate(transform.rotation);
            model->Draw({});
            if(turn){
                raylib::BoundingBox box = model->GetTransformedBoundingBox();
                box.Draw(RED);
            }
            model->transform = backupTransform;
        }
    }
    void Cleanup() override{}
};

struct StatComponent: public Component{
    int maxHP;
    int currentHP;
    int attack;
    void Setup() override{}
    void Update(float dt) override{}
    void Cleanup() override{}
    bool isFainted(){
        return currentHP <= 0;
    }
    void takeDamage(int dmg){
        currentHP -= dmg;
        if(currentHP < 0){
            currentHP = 0;
        }
    }
};

struct Move{
    enum Type{
        special,
        status,
    };
    std::string name;
    Type type;
    int power;
    int buff;
    int heal;
};

struct MovesetComponent: public Component{
    std::vector<Move> moveset;
    void Setup() override{}
    void Update(float dt) override{}
    void Cleanup() override{}
    Move* getMove(int i){
        return &moveset[i];
    }
};

void moveCalc(Entity& attacker, Entity& defender, Move& move){
    auto& attackerStats = attacker.GetComponent<StatComponent>()->get();
    auto& defenderStats = defender.GetComponent<StatComponent>()->get();
    if(move.type == Move::special){
        int dmg = attackerStats.attack * move.power;
        defenderStats.takeDamage(dmg);
    }
    if(move.type == Move::status){
        attackerStats.attack += move.buff;
        attackerStats.currentHP += move.heal;
    }
}

struct BattleStateMachine{
    enum Turn{
        Player,
        NPC,
        NJW,
        PEW,
    } turn = Player;
    Turn nextTurn = Player;
    float delay = 0;
    void update(Entity& p, Entity& n, raylib::Sound& sfx, float dt){
        auto& playerStats = p.GetComponent<StatComponent>()->get();
        auto& npcStats = n.GetComponent<StatComponent>()->get();
        if(delay > 0){
            delay -= dt;
            if (delay <= 0) {
                turn = nextTurn;
            }
            return;
        }
        if(npcStats.isFainted()){
            turn = NJW;
            return;
        }
        if(playerStats.isFainted()){
            turn = PEW;
            return;
        }
        if(turn == NPC){
            auto& moves = n.GetComponent<MovesetComponent>()->get().moveset;
            moveCalc(n, p, moves[0]);
            sfx.Play();
            nextTurn = Player;
            delay = 5.0f;
        }
    }
    void playerMoveSelector(Entity& p, Entity& n, raylib::Sound& sfx){
        if (delay > 0) return;
        auto& moves = p.GetComponent<MovesetComponent>()->get().moveset;
        if(IsKeyPressed(KEY_ONE)){
            moveCalc(p, n, moves[0]);
            sfx.Play();
            nextTurn = NPC;
            delay = 4.0f;
        }
        if(IsKeyPressed(KEY_TWO)){
            moveCalc(p, n, moves[1]);
            sfx.Play();
            nextTurn = NPC;
            delay = 4.0f;
        }
        if(IsKeyPressed(KEY_THREE)){
            moveCalc(p, n, moves[2]);
            sfx.Play();
            nextTurn = NPC;
            delay = 4.0f;
        }
    }
};

struct SceneStateMachine{
    enum Scene{
        Title,
        Battle,
        End,
    } scene = Title;
    void update(){
        switch(scene){
            case Title:
                updateTitle();
                break;
            case Battle:
                break;
            case End:
                break;
        }
    }
    void updateTitle(){
        if(raylib::Keyboard::IsKeyPressed(KEY_ENTER)){
            scene = Battle;
        }
    }
};

//Simple title screen
void DrawTitleScreen(raylib::Window& w){
    w.ClearBackground(raylib::Color::Gray());
    raylib::Text title;
    title.SetText("! ENCOUNTER !");
    title.SetFontSize(45);
    title.SetSpacing(5);
    title.SetColor(raylib::Color::Red());
    Vector2 titleSize = title.MeasureEx()/2;
    float titleX = w.GetWidth()/2 - titleSize.x;
    title.Draw({titleX, 50});

    raylib::Text sub;
    sub.SetText("The Miracle of Ironblood");
    sub.SetFontSize(20);
    sub.SetSpacing(2);
    sub.SetColor(raylib::Color::Black());
    Vector2 subSize = sub.MeasureEx()/2;
    float subX = w.GetWidth()/2 - subSize.x;
    sub.Draw({subX, 250});

    raylib::Text boss;
    boss.SetText("Prinz Eugen");
    boss.SetFontSize(75);
    boss.SetSpacing(2);
    boss.SetColor(raylib::Color::Black());
    Vector2 bossSize = boss.MeasureEx()/2;
    float bossX = w.GetWidth()/2 - bossSize.x;
    float bossY = w.GetHeight()/2 - bossSize.y;
    boss.Draw({bossX, bossY});

    raylib::Text start;
    start.SetText("<<< PRESS ENTER >>>");
    start.SetFontSize(25);
    start.SetSpacing(2);
    start.SetColor(raylib::Color::Orange());
    Vector2 startSize = start.MeasureEx()/2;
    float startX = w.GetWidth()/2 - startSize.x;
    start.Draw(startX, 550);
}

//End screen displays the name of the winner and a quote
//Quotes come from their respective MVP voicelines in Azur Lane
void DrawEndScreen(raylib::Window& w, BattleStateMachine::Turn& t){
    w.ClearBackground(raylib::Color::Gray());
    raylib::Text v;
    raylib::Text quote;
    if(t == BattleStateMachine::NJW){
        v.SetText("New Jersey Wins");
        v.SetFontSize(75);
        v.SetSpacing(2);
        v.SetColor(raylib::Color::Black());
        Vector2 vSize = v.MeasureEx()/2;
        float vX = w.GetWidth()/2 - vSize.x;
        float vY = w.GetHeight()/2 - vSize.y;
        v.Draw({vX, vY});
        quote.SetText("Course: steady! Objective: completed! Enemies who can stop us: none! Heheh, don'tcha love it when things go smoothly?");
        quote.SetFontSize(15);
        quote.SetSpacing(2);
        quote.SetColor(raylib::Color::Black());
        Vector2 quoteSize = quote.MeasureEx()/2;
        float quoteX = w.GetWidth()/2 - quoteSize.x;
        float quoteY = w.GetHeight()/2 - quoteSize.y;
        quote.Draw({quoteX, quoteY+75});
    }
    if(t == BattleStateMachine::PEW){
        v.SetText("Prinz Eugen Wins");
        v.SetFontSize(75);
        v.SetSpacing(2);
        v.SetColor(raylib::Color::Red());
        Vector2 vSize = v.MeasureEx()/2;
        float vX = w.GetWidth()/2 - vSize.x;
        float vY = w.GetHeight()/2 - vSize.y;
        v.Draw({vX, vY});
        quote.SetText("*Yawns* ... Battleships? Nothing but small fry.");
        quote.SetFontSize(15);
        quote.SetSpacing(2);
        quote.SetColor(raylib::Color::Red());
        Vector2 quoteSize = quote.MeasureEx()/2;
        float quoteX = w.GetWidth()/2 - quoteSize.x;
        float quoteY = w.GetHeight()/2 - quoteSize.y;
        quote.Draw({quoteX, quoteY+75});
    }
}

int main(){
    raylib::Window window(1000, 600, "As6");
    window.SetState(FLAG_WINDOW_RESIZABLE);
    raylib::AudioDevice audio;
    raylib::Camera camera({0, 120, 500}, {0, 0, 0});

    //set state for title and battle screen
    SceneStateMachine scene;
    BattleStateMachine battle;
    
    //import models for New Jersey(nj) and Prinz Eugen(pe)
    raylib::Model nj("models/nj.glb");
    raylib::Model pe("models/pe.glb");
    //scale and rotate the models
    nj.transform = raylib::Transform(nj.transform).Scale(100).RotateY(raylib::Degree(155));
    pe.transform = raylib::Transform(pe.transform).Scale(100).RotateY(raylib::Degree(-35));

    //import audio files
    raylib::Sound njSFX("audio/New_Jersey_SkillActivationJP.wav");
    raylib::Sound peSFX("audio/Prinz_Eugen_SkillActivationJP.wav");

    std::vector<Entity> entities;
    entities.reserve(10);
    //create an entity for New Jersey(NJ)
    auto& NJ = entities.emplace_back();
    //transform the New Jersey entity
    auto& NJtransform = NJ.AddComponent<TransformComponent>();
    NJtransform.position = raylib::Vector3{-175, -175, 0};
    auto& NJmodel = NJ.AddComponent<DrawModelComponent>();
    NJmodel.model = &nj;
    //give the New Jersey entity a statline
    auto& NJstats = NJ.AddComponent<StatComponent>();
    NJstats.maxHP = 8619;
    NJstats.currentHP = NJstats.maxHP;
    NJstats.attack = 40;
    //give the New Jersey entity a moveset (1 attacking move, 1 buffing move, and 1 healing move)
    auto& NJmoveset = NJ.AddComponent<MovesetComponent>();
    NJmoveset.moveset.push_back({"Dragon's Breath", Move::special, 50, 0, 0});
    NJmoveset.moveset.push_back({"Freedom Through Firepower", Move::status, 0, 15, 0});
    NJmoveset.moveset.push_back({"Don'tcha Just Love It?", Move::status, 0, 0, 1000});

    //create an entity for Prinz Eugen(PE)
    auto& PE = entities.emplace_back();
    //transform the Prinz Eugen entity
    auto& PEtransform = PE.AddComponent<TransformComponent>();
    PEtransform.position = raylib::Vector3{120, -25, 0};
    auto& PEmodel = PE.AddComponent<DrawModelComponent>();
    PEmodel.model = &pe;
    //give the Prinz Eugen entity a statline
    auto& PEstats = PE.AddComponent<StatComponent>();
    PEstats.maxHP = 5683;
    PEstats.currentHP = PEstats.maxHP;
    PEstats.attack = 30;
    //give the Prinz Eugen entity a moveset (1 attacking move)
    auto& PEmoveset = PE.AddComponent<MovesetComponent>();
    PEmoveset.moveset.push_back({"All Out Assault", Move::special, 40, 0, 0});

    for(Entity& e: entities){
        for(std::shared_ptr<Component>& c: e.components){
            c->Setup();
        }
    }

    while(!window.ShouldClose()){
        scene.update();
        if(scene.scene == SceneStateMachine::Battle){
            float dt = window.GetFrameTime();
            battle.update(NJ, PE, peSFX, dt);
        }
        NJmodel.turn = (battle.turn == BattleStateMachine::Player);
        PEmodel.turn = (battle.turn == BattleStateMachine::NPC);
        window.BeginDrawing();{
            if(scene.scene == SceneStateMachine::Title){
                DrawTitleScreen(window);
            }
            if(scene.scene == SceneStateMachine::Battle){
                window.ClearBackground(raylib::Color(15, 190, 209));
                float dt = window.GetFrameTime();
                camera.BeginMode();{
                    for(Entity& e: entities){
                        for(std::shared_ptr<Component>& c: e.components){
                            c->Update(dt);
                        }
                    }
                } camera.EndMode();
                window.DrawFPS();
                //Find New Jersey's model location to place text relative to it
                raylib::Vector3 NJWorldPos = NJtransform.position;
                raylib::Vector2 NJScreenPos = camera.GetWorldToScreen(NJWorldPos);
                const float NJX = NJScreenPos.x;
                const float NJY = NJScreenPos.y;
                //Display New Jersey's name and level
                raylib::Text playerKansen;
                playerKansen.SetText("New Jersey     Lv.100");
                playerKansen.SetFontSize(25);
                playerKansen.SetSpacing(2);
                playerKansen.SetColor(raylib::Color::Black());
                playerKansen.Draw(NJX+250, NJY-150);
                //Display New Jersey's HP (Current/Max)
                raylib::Text playerHP;
                playerHP.SetText("HP: " + std::to_string(NJstats.currentHP) + "/" + std::to_string(NJstats.maxHP));
                playerHP.SetFontSize(15);
                playerHP.SetSpacing(2);
                playerHP.SetColor(raylib::Color::Black());
                playerHP.Draw(NJX+395, NJY-125);

                //Find Prinz Eugen's model location to place text relalive to it
                raylib::Vector3 PEWorldPos = PEtransform.position;
                raylib::Vector2 PEScreenPos = camera.GetWorldToScreen(PEWorldPos);
                const float PEX = PEScreenPos.x;
                const float PEY = PEScreenPos.y;
                //Display Prinz Eugen's name and level
                raylib::Text npcKansen;
                npcKansen.SetText("Prinz Eugen     Lv.100");
                npcKansen.SetFontSize(25);
                npcKansen.SetSpacing(2);
                npcKansen.SetColor(raylib::Color::Black());
                npcKansen.Draw(PEX-450, PEY-250);
                //Display Prinz Eugen's HP (Current/Max)
                raylib::Text npcHP;
                npcHP.SetText("HP: " + std::to_string(PEstats.currentHP) + "/" + std::to_string(PEstats.maxHP));
                npcHP.SetFontSize(15);
                npcHP.SetSpacing(2);
                npcHP.SetColor(raylib::Color::Black());
                npcHP.Draw(PEX-300, PEY-225);

                if(battle.turn == BattleStateMachine::Player){
                    auto& moveset = NJmoveset.moveset;
                    //Draw New Jersey's First Move (Dragon's Breath)
                    raylib::Text move1;
                    move1.SetText("<<<1>>> " + moveset[0].name + " [SPECIAL]");
                    move1.SetFontSize(25);
                    move1.SetSpacing(2);
                    move1.SetColor(raylib::Color::Gray());
                    move1.Draw(NJScreenPos.x+125, NJScreenPos.y-95);
                    //Draw New Jersey's Second Move (Freedom Through Firepower)
                    raylib::Text move2;
                    move2.SetText("<<<2>>> " + moveset[1].name + " [BUFF]");
                    move2.SetFontSize(25);
                    move2.SetSpacing(2);
                    move2.SetColor(raylib::Color::Gray());
                    move2.Draw(NJScreenPos.x+125, NJScreenPos.y-70);
                    //Draw New Jersey's Third Move (Don'tcha Just Love It?)
                    raylib::Text move3;
                    move3.SetText("<<<3>>> " + moveset[2].name + " [HEAL]");
                    move3.SetFontSize(25);
                    move3.SetSpacing(2);
                    move3.SetColor(raylib::Color::Gray());
                    move3.Draw(NJScreenPos.x+125, NJScreenPos.y-45);

                    battle.playerMoveSelector(NJ, PE, njSFX);
                }
                if(battle.turn == BattleStateMachine::NJW || battle.turn == BattleStateMachine::PEW){
                    scene.scene = SceneStateMachine::End;
                }
            }
            if(scene.scene == SceneStateMachine::End){
                DrawEndScreen(window, battle.turn);
            }
        } window.EndDrawing();
    }
    
    for(Entity& e: entities){
        for(std::shared_ptr<Component>& c: e.components){
            c->Cleanup();
        }
    }
}