#include "callater.c"
#include "callater.h"
#include "game.c"
#include "raylib.h"
#include "gameobjects/enemy.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

DECL_GAMEOBJECT(Inspector, struct{ Enemy **enemies; uint32_t enemyTag; float width; CallaterRef enemyRefs[32]; });

#define GAMEOBJECT_TYPE Inspector
#include "gameobject.h"

bool subtracted = false;

static void Init(GameObject *go, void *arg)
{
    Inspector *inspector = (Inspector*) go;
    inspector->data.enemies = NULL;
    inspector->data.enemyTag = NameToTag("Enemy");
    inspector->data.width = 255;
    if(!subtracted)
    {
        subtracted = true;
        windowWidth -= inspector->data.width;
    }
}

static void Update(GameObject *go)
{
    if(IsKeyPressed(KEY_B))
    {
        __builtin_debugtrap();
    }
}

void ShowEnemyInvokes(GameObject *go)
{
    Inspector *inspector = (Inspector*) go;
    GameObjectGroup *group = &gameState.gameObjectGroups[inspector->data.enemyTag];
    
    arrsetlen(inspector->data.enemies, 0);
    
    for(uint64_t i = 0 ; i < group->count ; i++)
    {
        arrput(inspector->data.enemies, (Enemy*)group->objs[i]);
    }
    
    float rectHeight = 55.0f;
    for(uint64_t i = 0, len = arrlen(inspector->data.enemies) ; i < len ; i++)
    {
        uint64_t nbFound = CallaterGetGroupRefs(inspector->data.enemyRefs + i, inspector->data.enemies[i]->gameObjectHeader.id);
        
        if(nbFound == 0)
        {
            __builtin_debugtrap();
            CallaterGetGroupRefs(inspector->data.enemyRefs + i, inspector->data.enemies[i]->gameObjectHeader.id);
        }
        Vector2 start = {windowWidth, i * rectHeight};
        Rectangle rect = {
            .x = start.x,
            .y = start.y,
            .width = inspector->data.width,
            .height = rectHeight
        };
        DrawRectangleRec(rect, WHITE);
        DrawRectangleLinesEx(rect, 1.5f, PURPLE);
        
        CallaterRef ref = inspector->data.enemyRefs[i];
        char txt[256] = {0};
        sprintf(txt, "E:%zu FR:%.3f INV:%zu", inspector->data.enemies[i]->gameObjectHeader.id, inspector->data.enemies[i]->data.fireRate, ref.ref);
        DrawTextEx(GetFontDefault(), txt, (Vector2){rect.x + 10.0f, rect.y + rect.height / 2.0f}, 24.0f, 1.5f, BLACK);
        
        DrawRectangle(rect.x + 180.0f, rect.y, 25, 12, inspector->data.enemies[i]->data.shootToggle ? RED : BLUE);
    }
}

static void Draw(GameObject *go)
{
    ShowEnemyInvokes(go);
}

static void Deinit(GameObject *go)
{
    ;
}
