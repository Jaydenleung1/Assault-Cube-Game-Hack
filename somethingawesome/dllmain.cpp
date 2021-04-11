// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#pragma comment(lib, "OpenGL32.lib")
#include <gl/GL.h>

#define degToRad(angleInDegrees) ((angleInDegrees) * M_PI / 180.0)
#define radToDeg(angleInRadians) ((angleInRadians) * 180.0 / M_PI)

#define LOCAL_PLAYER_OFFSET 0x10F4F4
#define ENTITY_LIST_OFFSET 0x10F4F8
#define MAX_NUM_ENTITY_OFFSET 0x10F500
#define VIEWMATRIX_OFFSET 0X101AE8
#define WGLSWAPBUF_STOLEN_BYTES_SIZE 5
#define PLAYER_WIDTH 1.5f

// MENU ESP
#define MENU_ENEMY_ONLY "ESP Enemy Only"
#define MENU_BOX "ESP Box"
#define MENU_NAME "ESP Name"
#define MENU_HEALTH "ESP Health Bar"
#define MENU_ARMOR "ESP Armor Bar"
#define MENU_WEAPON_INFO "ESP Weapon Info"
#define MENU_TRACERS "ESP Tracers"

// MENU LOCAL PLAYER
#define MENU_MAX_HEALTH "Local Max Health"
#define MENU_MAX_ARMOR "Local Max Armor"
#define MENU_MAX_AMMO "Local Max Ammo"

class color {
public:
    float r, g, b;
    color() {};
    color(const float r, const float g, const float b) : r(r), g(g), b(b) {}
};

class vec3 {
public:
    float x, y, z;

    vec3() {};
    vec3(const float x, const float y, const float z) : x(x), y(y), z(z) {}
    vec3(const float x, const float y) : x(x), y(y), z(0.f) {}
    vec3 operator + (const vec3& rhs) const { return vec3(x + rhs.x, y + rhs.y, z + rhs.z); }
    vec3 operator - (const vec3& rhs) const { return vec3(x - rhs.x, y - rhs.y, z - rhs.z); }
    vec3 operator * (const float& rhs) const { return vec3(x * rhs, y * rhs, z * rhs); }
    vec3 operator / (const float& rhs) const { return vec3(x / rhs, y / rhs, z / rhs); }
    vec3& operator += (const vec3& rhs) { return *this = *this + rhs; }
    vec3& operator -= (const vec3& rhs) { return *this = *this - rhs; }
    vec3& operator *= (const float& rhs) { return *this = *this * rhs; }
    vec3& operator /= (const float& rhs) { return *this = *this / rhs; }
    float Length() const { return sqrtf(x * x + y * y + z * z); }
    vec3 Normalize() const { return *this * (1 / Length()); }
    float Distance(const vec3& rhs) const { return (*this - rhs).Length(); }
};

struct vec4 {
    float x, y, z, w;
};

namespace MATH {
    vec3 rotate_3d(vec3 vec, float distance,  float yaw) {
        vec.x = vec.x + cos(degToRad(yaw)) * distance;
        vec.y = vec.y + sin(degToRad(yaw)) * distance;
        return vec;
    }
}



class Entity
{
public:
    char pad_0000[4]; //0x0000
    vec3 chest; //0x0004
    char pad_0010[36]; //0x0010
    vec3 feetPos; //0x0034
    float yaw; //0x0040
    float pitch; //0x0044
    char pad_0048[176]; //0x0048
    int32_t health; //0x00F8
    int32_t armor; //0x00FC
    char pad_0100[293]; //0x0100
    char name[16]; //0x0225
    char pad_0235[247]; //0x0235
    int32_t team; //0x032C
    char pad_0330[68]; //0x0330
    class Weapon* currentWpn; //0x0374
}; //Size: 0x0378

// This is probably the most ridiculous way to do this but meh
class EntityData : public Entity {
public:
    struct Box {
        vec3 topLeft, botRight; // We don't need the other two corners because topLeft, botRight already contain them
        bool visible;
    };

    Entity *entity = nullptr;
    Box box;

    void init_box();
    bool is_enemy();
    vec3 get_head();
};

class Weapon
{
public:
    char pad_0000[12]; //0x0000
    class WeaponType* weaponType; //0x000C
    class Ammo* ammoReserve; //0x0010
    class Ammo* ammoMain; //0x0014
}; //Size: 0x0018

class Ammo
{
public:
    int32_t ammo; //0x0000
}; //Size: 0x0004


class WeaponType
{
public:
    char name[16]; //0x0000
    char pad_0010[264]; //0x0010
    int16_t maxAmmoCapacity; //0x0118
}; //Size: 0x011A

namespace GL {

    void setup_orth();
    void restore_orth();
    void draw_outline_rect(float x1, float y1, float x2, float y2, float lineWidth, color color);
    void draw_filled_rect(float x1, float y1, float x2, float y2, color color);
    void draw_dot(float x, float y, color color);
    void draw_line(float x1, float y1, float x2, float y2, color color);

    color red = { 255, 0, 0 };
    color green = { 0, 255, 0 };
    color blue = { 0, 0, 255 };
    color white = { 255, 255, 255 };
    color black = { 0, 0, 0 };
    int screenWidth = 0;
    int screenHeight = 0;

    class Font {
        private:
            int height;
            unsigned int base;
        public: 
            bool builtFont = false;
            HDC hdc = nullptr;

            void set_height(int h);
            int get_height();

            void build_font();
            void draw_text(float x, float y, color color, const char *format, ...);
    };
}

namespace Hook {
    bool Detour32(BYTE* src, BYTE* dst, const uintptr_t len);
    BYTE* TrampHook32(BYTE* src, BYTE* dst, const uintptr_t len);
    void Unhook32(BYTE* dst, BYTE* src, unsigned int size);
}

namespace ESP {
    void draw();
    bool world_to_screen(vec3 pos, vec3& screen, float viewMatrix[16], int windowWidth, int windowHeight);

    void draw_name(EntityData *currentPlayerData);
    void draw_bounding_box(EntityData *currentPlayerData);
    void draw_health_box(EntityData *currentPlayerData);
    void draw_armor_box(EntityData *currentPlayerData);
    void draw_weapon_info(EntityData *currentPlayerData);
    void draw_tracers(EntityData* currentPlayerData);
}

namespace GLOBAL {
    void init();

    uintptr_t moduleBaseAddy = 0;

    uintptr_t* localPlayerPtr = nullptr;
    uintptr_t* entityListPtr = nullptr;
    uintptr_t* maxEntitiesPtr = nullptr;
    uintptr_t* viewMatrixPtr = nullptr;

    Entity* localPlayer = nullptr;

    int32_t maxEntities = 0;

    uintptr_t* entityList = nullptr;

    float* viewMatrix = nullptr;

}

namespace LOCAL {
    void init();
    void max_health();
    void max_ammo();
    void max_armor();
}

class MenuItem {
public:
    char name[100] = { 0 };
    bool enabled = false;
    MenuItem() {};
    MenuItem(const char* n, const bool e) {
        strcpy(name, n);
        //name = strdup(n);
        enabled = e;
    }

    static bool is_enabled(const char* name);
};

///////////////////////////////

// From IDA: int __stdcall wglSwapBuffers(HDC hdc)
typedef BOOL(__stdcall* twglSwapBuffers) (HDC hdc);
twglSwapBuffers owglSwapBuffers;
GL::Font glFont;
std::mutex wglswapbufLock;
std::list <MenuItem> menu;
int select = 0;

void init_menu() {
    // Local player features
    menu.push_back(MenuItem(MENU_MAX_HEALTH, false));
    menu.push_back(MenuItem(MENU_MAX_ARMOR, false));
    menu.push_back(MenuItem(MENU_MAX_AMMO, false));
    
    // ESP features
    menu.push_back(MenuItem(MENU_ENEMY_ONLY, false));
    menu.push_back(MenuItem(MENU_NAME, false));
    menu.push_back(MenuItem(MENU_BOX, false));
    menu.push_back(MenuItem(MENU_HEALTH, false));
    menu.push_back(MenuItem(MENU_ARMOR, false));
    menu.push_back(MenuItem(MENU_WEAPON_INFO, false));
    menu.push_back(MenuItem(MENU_TRACERS, false));
}

BOOL __stdcall hook_wglSwapBuffers(HDC hdc) {

    wglswapbufLock.lock();

    GLOBAL::init();
    LOCAL::init();

    if (!glFont.builtFont || hdc != glFont.hdc) {
        glFont.set_height(15);
        glFont.build_font();
    }

    GL::setup_orth();
    ESP::draw();
    GL::restore_orth();

    wglswapbufLock.unlock();

    return owglSwapBuffers(hdc); // Jump to the stolen bytes, we need to pass args incase the stolen bytes use it
}



DWORD WINAPI thread(HMODULE hModule)
{
    // Start up a console
    AllocConsole();
    FILE *fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    
    // Setup Menu
    init_menu();

    // Get the address of where we want to hook
    BYTE *wglSwapBuffersAddy = (BYTE*)GetProcAddress(GetModuleHandle(L"opengl32.dll"), "wglSwapBuffers");
    
    // Hook it and return address of the stolen bytes
    owglSwapBuffers = (twglSwapBuffers)Hook::TrampHook32(wglSwapBuffersAddy, (BYTE*)hook_wglSwapBuffers, WGLSWAPBUF_STOLEN_BYTES_SIZE);

    // Endless loop
    while (!GetAsyncKeyState(VK_DELETE) & 1) {
        Sleep(5);
    }

    wglswapbufLock.lock();
    Hook::Unhook32(wglSwapBuffersAddy, (BYTE*)owglSwapBuffers, WGLSWAPBUF_STOLEN_BYTES_SIZE);
    wglswapbufLock.unlock();

    // Free console
    fclose(fp);
    FreeConsole();

    // Free the DLL
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CloseHandle(CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)thread, hModule, NULL, NULL));
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

bool Hook::Detour32(BYTE *src, BYTE *dst, const uintptr_t len) {
    // https://guidedhacking.com/threads/opengl-swapbuffers-hooking-for-drawings-and-etc.10943/
    // A jmp requires 5 bytes 
    if (len < 5) {
        return false;
    }

    // Since we are editing the code in the execution section of the code we need to call VirtualProtect to get the right permissions
    DWORD currProtection; // This stores the old permissions
    VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &currProtection);

    // Calculate the address of our function we want to call
    uintptr_t relativeAddress = dst - src - 5;

    // 0xE9 = JMP
    *src = 0xE9;

    *(uintptr_t*)(src + 1) = relativeAddress;

    // Restore the permissions so we don't break any other code
    VirtualProtect(src, len, currProtection, &currProtection);
    return true;

}

BYTE* Hook::TrampHook32(BYTE* src, BYTE* dst, const uintptr_t len) {
    // https://guidedhacking.com/threads/opengl-swapbuffers-hooking-for-drawings-and-etc.10943/
    if (len < 5) {
        return 0;
    }
    
    // Allocate some memory for our stolen bytes and jump back
    BYTE* gateway = (BYTE*)VirtualAlloc(0, len, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    // Copy the stolen bytes into memory
    memcpy_s(gateway, len, src, len);

    // Calculate the jump back address
    uintptr_t gatewayRelativeAddr = src - gateway - 5;

    // Add the jmp instruction
    *(gateway + len) = 0xE9;

    // Add where we want to jump to
    *(uintptr_t*)((uintptr_t)gateway + len + 1) = gatewayRelativeAddr;

    // Call Detour32
    Detour32(src, dst, len);

    return gateway;
}

void Hook::Unhook32(BYTE* dst, BYTE* src, unsigned int size) {
    // Change the permissions and store the old ones
    DWORD currProtection;
    if (VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &currProtection)) {
        // Copy the old bytes
        memcpy(dst, src, size);

        // Restore the permissions
        VirtualProtect(dst, size, currProtection, &currProtection);

        // Free the memory
        VirtualFree(owglSwapBuffers, 0, MEM_RELEASE);

        return;
    }
    printf("UNHOOK FAILED\n");
}

void GL::draw_outline_rect(float x1, float y1, float x2, float y2, float lineWidth, color color) {

    // More info here https://docs.microsoft.com/en-us/windows/win32/opengl/opengl

    // Set the type of the vertex we are drawing
    glBegin(GL_LINE_STRIP);

    // Set the width of the line
    glLineWidth(lineWidth);
    
    // Set the color of the line
    glColor3f(color.r, color.g, color.b);
    
    // glVertex2f work by setting the starting position and each time you call glVertex2f it draws from its current spot to the next spot
    // Set the starting pos to the top left corner
    glVertex2f(x1, y1);

    // Draw to top right corner
    glVertex2f(x2, y1);

    // Draw to bottom right corner
    glVertex2f(x2, y2);

    // Draw to bottom left corner
    glVertex2f(x1, y2);

    // Draw back to top left corner
    glVertex2f(x1, y1);

    glEnd();

}
void GL::draw_filled_rect(float x1, float y1, float x2, float y2, color color) {
    // Set the type of the vertex we are drawing
    glBegin(GL_POLYGON);

    // Set the color of the line
    glColor3f(color.r, color.g, color.b);

    // glVertex2f work by setting the starting position and each time you call glVertex2f it draws from its current spot to the next spot
    // Set the starting pos to the top left corner
    glVertex2f(x1, y1);

    // Draw to top right corner
    glVertex2f(x2, y1);

    // Draw to bottom right corner
    glVertex2f(x2, y2);

    // Draw to bottom left corner
    glVertex2f(x1, y2);

    // Draw back to top left corner
    glVertex2f(x1, y1);

    glEnd();
}

void GL::draw_line(float x1, float y1, float x2, float y2, color color) {

    // Set the type of the vertex we are drawing
    glBegin(GL_LINE_STRIP);

    // Set the width of the line
    glLineWidth(1);

    glColor3f(color.r, color.g, color.b);

    glVertex2f(x1, y1);

    glVertex2f(x2, y2);

    glEnd();
}

void GL::draw_dot(float x, float y, color color) {
    const int size = 5;
    // Set the type of the vertex we are drawing
    glBegin(GL_POLYGON);
    // Set the color of the line
    glColor3f(color.r, color.g, color.b);

    glVertex2f(x, y);

    // Draw to top right corner
    glVertex2f(x + size, y);

    // Draw to bottom right corner
    glVertex2f(x + size, y + size);

    // Draw to bottom left corner
    glVertex2f(x, y + size);

    // Draw back to top left corner
    glVertex2f(x, y);

    glEnd();
}

void GL::setup_orth() {
    // https://guidedhacking.com/threads/how-to-get-started-with-opengl-hacks.11475/
    // Set up orthographic projection matrix
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushMatrix();
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    screenWidth = viewport[2];
    screenHeight = viewport[3];
    glViewport(0, 0, screenWidth, screenHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, viewport[2], viewport[3], 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
}


void GL::restore_orth() {
    // https://guidedhacking.com/threads/how-to-get-started-with-opengl-hacks.11475/
    // Restore original orthographic projection matrix
    glPopMatrix();
    glPopAttrib();
}

void GL::Font::build_font() {
    // Get HDC
    hdc = wglGetCurrentDC();
    base = glGenLists(96);

    // create the font
    HFONT hNewFont = CreateFontA(-height, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Aerial");
    
    // Save our old font and select our new font 
    HFONT hOldFont = (HFONT)SelectObject(hdc, hNewFont);

    // 32, 96 = the range of the characters
    wglUseFontBitmaps(hdc, 32, 96, base);
    
    // Load our old font
    SelectObject(hdc, hOldFont);
    
    // Delete our new font
    DeleteObject(hNewFont);

    builtFont = true;
}

void GL::Font::draw_text(float x, float y, color color, const char* format, ...) {

    // Set color
    glColor3f(color.r, color.g, color.b);

    // Set location where to draw
    glRasterPos2f(x, y);

    // Create a buffer
    char text[100];
    va_list args;

    // Load buffer
    va_start(args, format);
    vsprintf_s(text, 100, format, args);
    va_end(args);

    glPushAttrib(GL_LIST_BIT);
    glListBase(base - 32);
    glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
    glPopAttrib();

}

void GL::Font::set_height(int h) {
    height = h;
}

int GL::Font::get_height() {
    return height;
}

bool ESP::world_to_screen(vec3 pos, vec3& screen, float viewMatrix[16], int windowWidth, int windowHeight) {
    // https://guidedhacking.com/threads/so-what-is-a-viewmatrix-anyway-and-how-does-a-w2s-work.10964/

    vec4 clipCoords;
    clipCoords.x = pos.x * viewMatrix[0] + pos.y * viewMatrix[4] + pos.z * viewMatrix[8] + viewMatrix[12];
    clipCoords.y = pos.x * viewMatrix[1] + pos.y * viewMatrix[5] + pos.z * viewMatrix[9] + viewMatrix[13];
    clipCoords.z = pos.x * viewMatrix[2] + pos.y * viewMatrix[6] + pos.z * viewMatrix[10] + viewMatrix[14];
    clipCoords.w = pos.x * viewMatrix[3] + pos.y * viewMatrix[7] + pos.z * viewMatrix[11] + viewMatrix[15];

    if (clipCoords.w < 0.1f) {
        return false;
    }
    
    vec3 NDC;
    NDC.x = clipCoords.x / clipCoords.w;
    NDC.y = clipCoords.y / clipCoords.w;
    NDC.z = clipCoords.z / clipCoords.w;

    
    screen.x = (windowWidth / 2 * NDC.x) + (NDC.x + windowWidth / 2);
    screen.y = -(windowHeight / 2 * NDC.y) + (NDC.y + windowHeight / 2);
    return true;
}

void GLOBAL::init() {

    // Get module base address
    moduleBaseAddy = (uintptr_t)GetModuleHandle(L"ac_client.exe");

    // Get pointers stuff
    localPlayerPtr = (uintptr_t*)(moduleBaseAddy + LOCAL_PLAYER_OFFSET); // Gets the local player pointer
    entityListPtr = (uintptr_t*)(moduleBaseAddy + ENTITY_LIST_OFFSET);
    maxEntitiesPtr = (uintptr_t*)(moduleBaseAddy + MAX_NUM_ENTITY_OFFSET);
    viewMatrixPtr = (uintptr_t*)(moduleBaseAddy + VIEWMATRIX_OFFSET);

    // Get local player
    localPlayer = (Entity*)*localPlayerPtr; // Dereferences it and casts it into an entity pointer

    // Get the max number of entities
    maxEntities = (int32_t)*maxEntitiesPtr;

    // Get entity list
    entityList = (uintptr_t*)*entityListPtr;

    // Get view matrix
    viewMatrix = (float*)viewMatrixPtr;
}

bool EntityData::is_enemy() {
    if (GLOBAL::localPlayer == nullptr) {
        return false;
    }

    if (GLOBAL::localPlayer->team == this->entity->team) {
        return false;
    }

    return true;
}

vec3 EntityData::get_head() {
    vec3 head = this->entity->chest;
    head.z += 1;
    return head;
}

void EntityData::init_box() {
    if (GLOBAL::localPlayer == nullptr) {
        this->box.visible = false;
        return;
    }

    if (this->entity->health <= 0) {
        this->box.visible = false;
        return;
    }

    // Never again will I scale my drawing in 3D space. Just do it in 2D

    vec3 topleftVec = MATH::rotate_3d(this->get_head(), PLAYER_WIDTH, GLOBAL::localPlayer->yaw + 180);
    vec3 botrightVec = MATH::rotate_3d(this->entity->feetPos, PLAYER_WIDTH, GLOBAL::localPlayer->yaw);

    if (ESP::world_to_screen(botrightVec, this->box.botRight, GLOBAL::viewMatrix, GL::screenWidth, GL::screenHeight)) {
        if (ESP::world_to_screen(topleftVec, this->box.topLeft, GLOBAL::viewMatrix, GL::screenWidth, GL::screenHeight)) {
            this->box.visible = true;
            return;
        }
    }
    this->box.visible = false;
}



void ESP::draw() {

    // Draw Menu Stuff
    auto get_color = [](std::list<MenuItem>::iterator menu) {
        if (menu->enabled) {
            return color(0, 255, 0);
        }
        else {
            return color(255, 0, 0);
        }
    };

    for (std::list<MenuItem>::iterator it = menu.begin(); it != menu.end(); ++it) {
        auto i = std::distance(menu.begin(), it);
        int offset = (20 * i) + 120;

        if (select == i) {
            glFont.draw_text(20, offset, GL::white, "[x]");
        }

        glFont.draw_text(40, offset, get_color(it), "%s", it->name);

    }

    // Handle Menu Keybinds
    if (GetAsyncKeyState(VK_DOWN) & 1) {
        select++;
        select = select % menu.size();
    } else if (GetAsyncKeyState(VK_UP) & 1) {
        select--;
        if (select < 0) {
            select = menu.size() - 1;
        }
    } else if (GetAsyncKeyState(VK_LEFT) & 1) {
        auto item = menu.begin();
        std::advance(item, select);
        item->enabled = false;
    } else if (GetAsyncKeyState(VK_RIGHT) & 1) {
        auto item = menu.begin();
        std::advance(item, select);
        item->enabled = true;
    }

    // Draw Esp Stuff
    if (GLOBAL::entityList) {
        // i = 1 beacuse there is no 0 entity
        for (int32_t i = 1; i < GLOBAL::maxEntities; i++) {

            EntityData currentPlayerData;
            currentPlayerData.entity = (Entity*)*(GLOBAL::entityList + i);

            if (currentPlayerData.entity == nullptr) {
                continue;
            }

            if (MenuItem::is_enabled(MENU_ENEMY_ONLY) && !currentPlayerData.is_enemy()) {
                continue;
            }

            currentPlayerData.init_box();

            if (currentPlayerData.box.visible) {
                if (MenuItem::is_enabled(MENU_NAME)) {
                    ESP::draw_name(&currentPlayerData);
                }

                if (MenuItem::is_enabled(MENU_BOX)) {
                    ESP::draw_bounding_box(&currentPlayerData);
                }

                if (MenuItem::is_enabled(MENU_HEALTH)) {
                    ESP::draw_health_box(&currentPlayerData);
                }

                if (MenuItem::is_enabled(MENU_ARMOR)) {
                    ESP::draw_armor_box(&currentPlayerData);
                }

                if (MenuItem::is_enabled(MENU_WEAPON_INFO)) {
                    ESP::draw_weapon_info(&currentPlayerData);
                }

                if (MenuItem::is_enabled(MENU_TRACERS)) {
                    ESP::draw_tracers(&currentPlayerData);
                }
            }
        }
    }
}

void ESP::draw_bounding_box(EntityData *currentPlayerData) {
    GL::draw_outline_rect(currentPlayerData->box.topLeft.x, 
        currentPlayerData->box.topLeft.y, 
        currentPlayerData->box.botRight.x, 
        currentPlayerData->box.botRight.y, 
        1, 
        GL::red
    );
}

void ESP::draw_name(EntityData *currentPlayerData) {
    glFont.draw_text(currentPlayerData->box.topLeft.x, currentPlayerData->box.topLeft.y - 7, GL::white, "%s", currentPlayerData->entity->name);
}

void ESP::draw_health_box(EntityData* currentPlayerData) {
    const float offset = PLAYER_WIDTH + 0.1f;
    const float width = offset + 0.5f;
    float curHealth = currentPlayerData->entity->health;
    if (curHealth < 0) {
        curHealth = 0;
    }

    // Outline
    vec3 topleftVecScreen, botrightVecScreen;
    vec3 topleftVec = MATH::rotate_3d(currentPlayerData->get_head(), width, GLOBAL::localPlayer->yaw + 180);
    vec3 botrightVec = MATH::rotate_3d(currentPlayerData->get_head(), offset, GLOBAL::localPlayer->yaw + 180);
    if (ESP::world_to_screen(botrightVec, botrightVecScreen, GLOBAL::viewMatrix, GL::screenWidth, GL::screenHeight)) {
        if (ESP::world_to_screen(topleftVec, topleftVecScreen, GLOBAL::viewMatrix, GL::screenWidth, GL::screenHeight)) {
            GL::draw_outline_rect(topleftVecScreen.x,
                topleftVecScreen.y,
                botrightVecScreen.x,
                currentPlayerData->box.botRight.y,
                1,
                GL::black
            );
        }
    }


    // Red
    topleftVec = MATH::rotate_3d(currentPlayerData->get_head(), width, GLOBAL::localPlayer->yaw + 180);
    botrightVec = MATH::rotate_3d(currentPlayerData->get_head(), offset, GLOBAL::localPlayer->yaw + 180);
    if (ESP::world_to_screen(botrightVec, botrightVecScreen, GLOBAL::viewMatrix, GL::screenWidth, GL::screenHeight)) {
        if (ESP::world_to_screen(topleftVec, topleftVecScreen, GLOBAL::viewMatrix, GL::screenWidth, GL::screenHeight)) {
            GL::draw_filled_rect(topleftVecScreen.x,
                topleftVecScreen.y,
                botrightVecScreen.x,
                currentPlayerData->box.botRight.y,
                GL::red
            );
        }
    }

    // green
    vec3 leftishbotrightVecScreen;
    topleftVec = MATH::rotate_3d(currentPlayerData->get_head(), width, GLOBAL::localPlayer->yaw + 180);
    botrightVec = MATH::rotate_3d(currentPlayerData->get_head(), offset, GLOBAL::localPlayer->yaw + 180);
    if (ESP::world_to_screen(botrightVec, leftishbotrightVecScreen, GLOBAL::viewMatrix, GL::screenWidth, GL::screenHeight)) {
        if (ESP::world_to_screen(topleftVec, topleftVecScreen, GLOBAL::viewMatrix, GL::screenWidth, GL::screenHeight)) {
            float a = fabsf(currentPlayerData->box.botRight.y - topleftVecScreen.y);
            float b = (fabsf(currentPlayerData->box.botRight.y - topleftVecScreen.y) * curHealth);

            float height = a - (b / 100);
            
            GL::draw_filled_rect( topleftVecScreen.x,
                topleftVecScreen.y + height,
                leftishbotrightVecScreen.x,
                currentPlayerData->box.botRight.y,
                GL::green
            );
        }
    }
}

void ESP::draw_armor_box(EntityData* currentPlayerData) {
    const float offset = PLAYER_WIDTH + 0.1f;
    const float width = offset + 0.5f;
    float curArmor = currentPlayerData->entity->armor;
    if (curArmor < 0) {
        curArmor = 0;
    }

    // Outline
    vec3 topRightScreen, topRightLeftishScreen;
    vec3 topRight = MATH::rotate_3d(currentPlayerData->get_head(), width, GLOBAL::localPlayer->yaw);
    vec3 topRightLeftish = MATH::rotate_3d(currentPlayerData->get_head(), offset, GLOBAL::localPlayer->yaw);
    if (ESP::world_to_screen(topRight, topRightScreen, GLOBAL::viewMatrix, GL::screenWidth, GL::screenHeight)) {
        if (ESP::world_to_screen(topRightLeftish, topRightLeftishScreen, GLOBAL::viewMatrix, GL::screenWidth, GL::screenHeight)) {
            GL::draw_outline_rect(topRightScreen.x,
                topRightScreen.y,
                topRightLeftishScreen.x,
                currentPlayerData->box.botRight.y,
                1,
                GL::black
            );
        }
    }


    // blue
    if (ESP::world_to_screen(topRight, topRightScreen, GLOBAL::viewMatrix, GL::screenWidth, GL::screenHeight)) {
        if (ESP::world_to_screen(topRightLeftish, topRightLeftishScreen, GLOBAL::viewMatrix, GL::screenWidth, GL::screenHeight)) {

            float a = fabsf(currentPlayerData->box.botRight.y - topRightScreen.y);
            float b = (fabsf(currentPlayerData->box.botRight.y - topRightScreen.y) * curArmor);

            float height = a - (b / 100);

            GL::draw_filled_rect(topRightLeftishScreen.x,
                topRightLeftishScreen.y + height,
                topRightScreen.x,
                currentPlayerData->box.botRight.y,
                GL::blue
            );
        }
    }
}

void ESP::draw_weapon_info(EntityData* currentPlayerData) {

    glFont.draw_text(currentPlayerData->box.topLeft.x, 
        currentPlayerData->box.botRight.y + 10, 
        GL::white, "%s (%d:%d)", 
        currentPlayerData->entity->currentWpn->weaponType->name, 
        currentPlayerData->entity->currentWpn->ammoMain->ammo, 
        currentPlayerData->entity->currentWpn->weaponType->maxAmmoCapacity
    );
}

void ESP::draw_tracers(EntityData* currentPlayerData) {
    GL::draw_line(GL::screenWidth / 2, 
        GL::screenHeight, 
        fabsf((currentPlayerData->box.botRight.x + currentPlayerData->box.topLeft.x) / 2),
        currentPlayerData->box.botRight.y, 
        GL::red);
}

bool MenuItem::is_enabled(const char *name) {
    auto compare = [name](const MenuItem& m) {
        if (strcmp(m.name, name) == 0) {
            return true;
        }
        else {
            return false;
        }
    };
    // WTF is going on here
    auto it = std::find_if(menu.begin(), menu.end(), compare);
    return it->enabled;
};

void LOCAL::init() {
    if (MenuItem::is_enabled(MENU_MAX_HEALTH)) {
        LOCAL::max_health();
    }
    if (MenuItem::is_enabled(MENU_MAX_AMMO)) {
        LOCAL::max_ammo();
    }
    if (MenuItem::is_enabled(MENU_MAX_ARMOR)) {
        LOCAL::max_armor();
    }
}

void LOCAL::max_health() {
    if (GLOBAL::localPlayer == nullptr) {
        return;
    }
    GLOBAL::localPlayer->health = 999;
}

void LOCAL::max_ammo() {
    if (GLOBAL::localPlayer == nullptr) {
        return;
    }

    GLOBAL::localPlayer->currentWpn->ammoMain->ammo = 999;
    GLOBAL::localPlayer->currentWpn->ammoReserve->ammo = 999;
}

void LOCAL::max_armor() {
    if (GLOBAL::localPlayer == nullptr) {
        return;
    }
    GLOBAL::localPlayer->armor = 999;

}