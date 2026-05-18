#include "rlib/raylib.h"
#include "rlib/raymath.h"
#include "raylib_win32.h"
#include <tlhelp32.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#define DEBUG_MODE
size_t ConvertToFtime(size_t time){
    size_t elapsed_seconds = (size_t)((double)time / 1000.0f);
    return elapsed_seconds * 60;
}
int is_process_running(const char *process_name) {
    PROCESSENTRY32 pe32;
    HANDLE hProcessSnap;
    int found = 0;
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        printf("CreateToolhelp32Snapshot failed. Error: %lu\n", GetLastError());
        return 0;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (!Process32First(hProcessSnap, &pe32)) {
        printf("Process32First failed. Error: %lu\n", GetLastError());
        CloseHandle(hProcessSnap);
        return 0;
    }
    do {
        if (_stricmp(pe32.szExeFile, process_name) == 0) {
            found = 1;
            break;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return found;
}
DWORD get_process_id_by_name(const char *process_name) {
    PROCESSENTRY32 pe32;
    HANDLE hSnapshot;
    DWORD pid = 0;

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return 0;

    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (_stricmp(pe32.szExeFile, process_name) == 0) {
                pid = pe32.th32ProcessID;
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return pid;
}
int is_process_in_focus(const char *process_name) {
    HWND hwnd = GetForegroundWindow(); 
    if (hwnd == NULL)
        return 0;

    DWORD fg_pid;
    GetWindowThreadProcessId(hwnd, &fg_pid); 

    DWORD target_pid = get_process_id_by_name(process_name);
    if (target_pid == 0)
        return 0;

    return (fg_pid == target_pid);
}


typedef struct {
    size_t time_open;
    size_t time_in_focus;
    size_t time_doing_stuff;

    size_t ftime_open;
    size_t ftime_in_focus;
    size_t ftime_doing_stuff;
    const char *process_name;
    const char *app_name;
}AppInfo;
#define MAX_APPS 10
typedef struct{
    AppInfo app_array[MAX_APPS];
    size_t timer;
    size_t ftime;
}SaveInfo;
int WriteSaveInfo(SaveInfo *save_info){
    FILE *f = fopen("time.data", "wb");
    if (f == NULL){
        printf("Error opening file!\n");
        return 0;
    }
    size_t n = fwrite(save_info, 1 , sizeof(SaveInfo), f);
    printf("Saved data length: %lld\n", n);
    fclose(f);
    return n;
}
int WriteSaveInfoBackup(SaveInfo *save_info){
    FILE *f = fopen("backup/time.data", "wb");
    if (f == NULL){
        printf("Error opening file!\n");
        return 0;
    }
    size_t n = fwrite(save_info, 1 , sizeof(SaveInfo), f);
    printf("Saved data length: %lld\n", n);
    fclose(f);
    return n;
}
int LoadSaveInfo(SaveInfo *save_info){
    FILE *f = fopen("time.data", "rb");
    if (f == NULL){
        printf("Error opening file!\n");
        return 0;
    }
    fseek(f, 0, SEEK_END);          
    size_t flen = ftell(f);  
    rewind(f);
    fread(save_info, flen, 1, f);
    fclose(f);
    return 1;
}
void CleanSaveInfo(SaveInfo *save_info){
    for(int i =0;i<MAX_APPS;i++){
        save_info->app_array[i].time_doing_stuff = 0;
        save_info->app_array[i].time_in_focus = 0;
        save_info->app_array[i].time_open = 0;     

        save_info->app_array[i].ftime_doing_stuff = 0;
        save_info->app_array[i].ftime_in_focus = 0;
        save_info->app_array[i].ftime_open = 0;
    }
    save_info->timer = 0;
    save_info->ftime = 0;
}
float get_percent(float _value, float _max_value){
	return _value / _max_value;
}
//#define DEBUG_MODE 
#define ACTIONS_SLEEP_DELAY 15
typedef struct {
    unsigned char array[255];
    unsigned char len;
}KeyArray;
uint64_t *KeyTime(){
    static uint64_t key_pressed_time[UINT8_MAX] = {0};
    return key_pressed_time;
}
uint8_t *KeyPressed(){
    static unsigned char key_pressed[UINT8_MAX] = {0};
    return key_pressed;
}
int IsAnykeyPressed(){
    KeyArray key_array = {0};
    ZeroMemory(&key_array, sizeof(KeyArray));
    for(int i =1;i<254;i++){
        if( GetAsyncKeyState(i)){
           return 1;
        }
    }
    return 0;
}
static POINT mpos = {0};
static POINT prev_mpos = {0};
void UpdateRunningAppInfo(SaveInfo *save_info, const int i, const size_t tick_timer){
    static int time_key_press = 0;
    if( IsAnykeyPressed()){
        time_key_press = ACTIONS_SLEEP_DELAY;
    }
    if(time_key_press > 0){
        time_key_press--;
    }
    if(save_info->app_array[i].process_name != 0){
        if (is_process_running(save_info->app_array[i].process_name)) {
            save_info->app_array[i].time_open += tick_timer;
            //save_info->app_array[i].time_open =ConvertToFtime( save_info->app_array[i].ftime_open );
            if (is_process_in_focus(save_info->app_array[i].process_name)) {
                save_info->app_array[i].time_in_focus+= tick_timer ;
                //save_info->app_array[i].time_in_focus =ConvertToFtime( save_info->app_array[i].ftime_in_focus );
                
                if(time_key_press > 0){
                    save_info->app_array[i].time_doing_stuff+= tick_timer ;
                    //save_info->app_array[i].time_doing_stuff =ConvertToFtime( save_info->app_array[i].ftime_doing_stuff );
                }
            }
        } 
    }
}
const char *FormatTimeAsTime(size_t time){
    int second = (int)(time/60);
    int minute = (int)(second/60);
    int hour = (int)(minute/60);
    return TextFormat( "%d::%d::%d", hour, minute % 60, second % 60);
}
static ULONGLONG start_time[5] = {0};
void UpdateCounter(int i){
    start_time[i] = GetTickCount64();
}   
ULONGLONG GetTimePassed(int i ){
    return GetTickCount64() - start_time[i];
}

//-mwindows
int main() {
    size_t project_time = 0;
    int last_update_procces = MAX_APPS;
    int update_tick_timer = 0;

    int font_size = 30;
    int is_clock_active = 0;
    Color circle_color = GREEN;
    int circle_range = 60;
    int circle_activation_time = 120;
    int circle_activation_timer = 0;
    int clear_activation_time = 160;
    int clear_activation_timer = 0;
    Vector2 mouse_pos = {0};
    int screen_w = 0;
    int screen_h = 0;
    int holding_the_circle = 0;
    int press_to_untag = 0;
    SaveInfo save_info = {0};
    if(!LoadSaveInfo(&save_info)){
        WriteSaveInfo(&save_info);
    }
  
    int holding_the_clear= 0;
    int counter = 0;
    save_info.app_array[counter].process_name = "cmd.exe"; 
    save_info.app_array[counter++].app_name = "Command line"; 
    save_info.app_array[counter].process_name = "GameMaker.exe"; 
    save_info.app_array[counter++].app_name = "GameMaker"; 
    save_info.app_array[counter].process_name = "devenv.exe"; //Visual Studio
    save_info.app_array[counter++].app_name = "Visual Studio"; //Visual Studio
    // save_info.app_array[counter].process_name = "chrome.exe"; //Chrome
    // save_info.app_array[counter++].app_name = "Chrome"; //Chrome
    save_info.app_array[counter].process_name = "MyDrugCartel.exe"; //Game
    save_info.app_array[counter++].app_name = "My Drug Cartel"; //Game
    save_info.app_array[counter].process_name = "Igor.exe"; //Game in debug mode
    save_info.app_array[counter++].app_name = "GML Debug Game"; //Game in debug mode
    save_info.app_array[counter].process_name = "Code.exe"; //Visual Studio Code
    save_info.app_array[counter++].app_name = "Visual Studio Code"; //Visual Studio Code
    int max_update_procces = counter;
    #ifdef DEBUG_MODE
        SetTraceLogLevel(LOG_ALL);
    #endif
    #ifndef DEBUG_MODE
        SetTraceLogLevel(LOG_NONE);
    #endif
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow( 700, 1280 * 0.5, "Timer");
    SetTargetFPS(60);
    while(!WindowShouldClose()){
        GetCursorPos(&mpos);
        circle_color = RED;
        mouse_pos = GetMousePosition();
        screen_w = GetScreenWidth();
        screen_h = GetScreenHeight();
        holding_the_circle= 0;
        holding_the_clear = 0;
        Vector2 screen_center = {.x = screen_w * 0.5, .y = screen_h * 0.5};
        Rectangle clear_rect = {.x = screen_w-16 - 220, .y = 16, .width = 220, .height = 50};
        if(is_clock_active){
            save_info.ftime += GetTimePassed(0);// GetTickCount64() - start_time;
            //size_t elapsed_seconds = (size_t)((double)save_info.ftime / 1000.0f);
            save_info.timer = ConvertToFtime(save_info.ftime);//elapsed_seconds * 60;
            circle_color = GREEN;
            printf("%zu\n", save_info.ftime);
        }
        UpdateCounter(0);
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))press_to_untag = 0;
        if(CheckCollisionPointRec(mouse_pos, clear_rect) && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !press_to_untag && !is_clock_active){
            holding_the_clear = 1;
            clear_activation_timer++;
            if(clear_activation_timer >= clear_activation_time){
                CleanSaveInfo(&save_info);
                WriteSaveInfo(&save_info);
                
                press_to_untag =1;
                clear_activation_timer=0;
            }
        }else {
            clear_activation_timer = 0;
        }
        if(CheckCollisionPointCircle(mouse_pos, screen_center, circle_range) && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !press_to_untag){
            holding_the_circle = 1;
            circle_activation_timer++;
            if(circle_activation_timer >= circle_activation_time){
                is_clock_active = !is_clock_active;
                press_to_untag =1;
                circle_activation_timer=0;
            }
        }else {
            circle_activation_timer = 0;
        }
        BeginDrawing();
            ClearBackground(BLACK);
            const char *timer_text = FormatTimeAsTime(save_info.timer);// TextFormat( "%d::%d::%d", hour, minute % 60, second % 60);
            int text_w = MeasureText(timer_text, font_size);

            DrawCircle( screen_center.x, screen_center.y, 60, circle_color);
            DrawRectangleRec(clear_rect, BLUE);
            if(holding_the_circle){
                if(is_clock_active)DrawCircle( screen_center.x, screen_center.y, circle_range * get_percent(circle_activation_timer, circle_activation_time), RED);
                else DrawCircle( screen_center.x, screen_center.y, circle_range* get_percent(circle_activation_timer, circle_activation_time), GREEN);
            } 
            if(holding_the_clear){
                DrawRectangle(clear_rect.x, clear_rect.y, clear_rect.width* get_percent(clear_activation_timer, clear_activation_time), clear_rect.height, RED);
            }
            
            DrawText("Start new timer", clear_rect.x+16, clear_rect.y+16, 24, WHITE);
            
            DrawText(timer_text, screen_center.x - (text_w*0.5), screen_center.y + (60+20), font_size, WHITE);
            #ifdef DEBUG_MODE 
                int h_offset =16;
                int text_size = 20;
                for(int i =0;i<max_update_procces;i++){
                    DrawText(TextFormat("[%s]:",save_info.app_array[i].app_name), 16,16+h_offset, text_size, WHITE);
                    h_offset+=text_size+1;
                    DrawText(TextFormat("Time opened: %s",FormatTimeAsTime(save_info.app_array[i].time_open)), 24,16+h_offset, text_size, WHITE);
                    h_offset+=text_size+1;
                    DrawText(TextFormat("Time in focus: %s",FormatTimeAsTime(save_info.app_array[i].time_in_focus)), 24,16+h_offset, text_size, WHITE);
                    h_offset+=text_size+1;
                    DrawText(TextFormat("Time doing something: %s",FormatTimeAsTime(save_info.app_array[i].time_doing_stuff)), 24,16+h_offset, text_size, WHITE);
                    h_offset+=text_size+1;
                    h_offset+=10;
                }
            #endif
         EndDrawing();
         if(is_clock_active){
            if(update_tick_timer >= 10){
                if(last_update_procces >= max_update_procces){
                    last_update_procces =0 ;
                }
                UpdateRunningAppInfo(&save_info, last_update_procces,   (const int)(10 * max_update_procces));
                last_update_procces+=1;
                update_tick_timer=0;
                //UpdateCounter(0);
            }
            update_tick_timer++;
        }
        prev_mpos.x = mpos.x;
        prev_mpos.y = mpos.y;
        project_time++;
        if(is_clock_active && !(project_time % 360)){
            WriteSaveInfo(&save_info);
        }
        if(IsKeyPressed(KEY_S))WriteSaveInfoBackup(&save_info);
    }
    WriteSaveInfo(&save_info);
    return 0;
}