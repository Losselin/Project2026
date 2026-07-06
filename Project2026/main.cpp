#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <graphics.h>
#include <cwchar>
#include <string>
#include <vector>
#include <fstream>
#include <cstring>
#include <algorithm>
#include "checkerboard.h"

#define FILE_NAME  "users.txt"
#define USERS_FILE "users.txt"

void drawTitle();

namespace {
    const int WINDOW_WIDTH = 1280;
    const int WINDOW_HEIGHT = 720;
    const int PANEL_LEFT = 50;
    const int PANEL_TOP = 100;
    const int PANEL_WIDTH = 520;
    const int PANEL_HEIGHT = 440;

    enum class Scene {
        Title,
        Main,
        Login,
        Register
    };

    enum class ActiveField {
        None,
        LoginUsername,
        LoginPassword,
        RegisterUsername,
        RegisterPassword
    };
    struct Button {
        RECT rect;
        const wchar_t* label;
        COLORREF color;
    };
    struct TextField {
        RECT rect;
        std::wstring text;
        std::wstring placeholder;
        bool password = false;
        size_t maxLength = 20;
        TextField(RECT r,std::wstring plh,bool pwd=false,size_t len=20)
            :rect(r),placeholder(plh),password(pwd),maxLength(len){ }
    };
    struct Account {
        std::wstring username;
        std::wstring password;
    };
    struct AppState {
        Scene scene = Scene::Title;
        ActiveField activeField = ActiveField::None;
        bool running = true;
        TextField loginUsername = { {PANEL_LEFT + 70,PANEL_TOP + 148,PANEL_LEFT + 390,PANEL_TOP + 204},L"请输入用户名",false,20 };
        TextField loginPassword = { {PANEL_LEFT + 70,PANEL_TOP + 248,PANEL_LEFT + 390,PANEL_TOP + 304},L"请输入密码",false,20 };
        TextField registerUsername = { {PANEL_LEFT + 70,PANEL_TOP + 148,PANEL_LEFT + 390,PANEL_TOP + 204},L"请输入用户名",false,20 };
        TextField registerPassword = { {PANEL_LEFT + 70,PANEL_TOP + 248,PANEL_LEFT + 390,PANEL_TOP + 304},L"请输入密码",false,20 };

        Button loginButton = { { PANEL_LEFT + 70, PANEL_TOP + 318, PANEL_LEFT + 220, PANEL_TOP + 376 }, L"登录", RGB(49, 121, 245) };
        Button toRegisterButton = { { PANEL_LEFT + 240, PANEL_TOP + 318, PANEL_LEFT + 390, PANEL_TOP + 376 }, L"注册", RGB(55, 167, 111) };
        Button registerSubmitButton = { { PANEL_LEFT + 70, PANEL_TOP + 318, PANEL_LEFT + 220, PANEL_TOP + 376 }, L"提交注册", RGB(55, 167, 111) };
        Button registerBackButton = { { PANEL_LEFT + 240, PANEL_TOP + 318, PANEL_LEFT + 390, PANEL_TOP + 376 }, L"返回", RGB(120, 132, 158) };
        Button logoutButton = { { PANEL_LEFT + 150, PANEL_TOP + 304, PANEL_LEFT + 350, PANEL_TOP + 362 }, L"退出登录", RGB(230, 150, 50) };
        Button exitButton = { { PANEL_LEFT + 390, PANEL_TOP + 304, PANEL_LEFT + 590, PANEL_TOP + 362 }, L"退出程序", RGB(208, 80, 80) };

        std::vector<Account> accounts;
        std::wstring currentUser;
        std::wstring statusMessage = L"请先注册或直接登录。";
    };

    bool HitTest(const RECT& rect, int x, int y){
        return x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom;
    }
    bool HitButton(const Button& button, int x, int y){
        return HitTest(button.rect, x, y);
    }

    bool IsAllowedInput(wchar_t ch) {
        return (ch >= L'0' && ch <= L'9') || (ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z') || ch == L'_';
    }

    bool FileExists(const std::wstring& path) {
        const DWORD attr = GetFileAttributesW(path.c_str());
        return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
    }

    std::wstring GetExeDir() {
        wchar_t modulePath[MAX_PATH] = {};
        GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
        std::wstring path(modulePath);
        const size_t slash = path.find_last_of(L"\\/");
        if (slash == std::wstring::npos) {
            return L".";
        }
        return path.substr(0, slash);
    }

    bool LoadTitleImage(IMAGE& image) {
        const std::wstring fileName = L"1594038326_159390.jpg";
        const std::wstring exeDir = GetExeDir();
        const std::vector<std::wstring> candidates = {
            exeDir + L"\\" + fileName,
            exeDir + L"\\..\\..\\..\\x64\\Debug\\" + fileName,
            L".\\x64\\Debug\\" + fileName,
            L"..\\x64\\Debug\\" + fileName,
            L"Project2026\\x64\\Debug\\" + fileName
        };

        for (const auto& path : candidates) {
            if (!FileExists(path)) {
                continue;
            }
            loadimage(&image, path.c_str(), WINDOW_WIDTH, WINDOW_HEIGHT, true);
            return true;
        }
        return false;
    }

    bool LoadAuthBgImage(IMAGE& image) {
        const std::vector<std::wstring> fileNames = {
            L"1553158497_845743.jpg",
            L"1553158497_845743.jpeg",
            L"1553158497_845743.png",
            L"1553158497_845743.bmp",
            L"登录注册.jpg",
            L"登录注册.jpeg",
            L"登录注册.png",
            L"登录注册.bmp"
        };
        const std::wstring exeDir = GetExeDir();
        for (const auto& fileName : fileNames) {//多路径加载图片
            const std::vector<std::wstring> candidates = {
                exeDir + L"\\" + fileName,
                exeDir + L"\\..\\..\\..\\x64\\Debug\\" + fileName,
                L".\\x64\\Debug\\" + fileName,
                L"..\\x64\\Debug\\" + fileName,
                L"Project2026\\x64\\Debug\\" + fileName
            };
            for (const auto& path : candidates) {
                if (!FileExists(path)) {
                    continue;
                }
                loadimage(&image, path.c_str(), WINDOW_WIDTH, WINDOW_HEIGHT, true);
                return true;
            }
        }
        return false;
    }

    std::string ToUtf8(const std::wstring& wstr) {
        if (wstr.empty()) return "";
        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
        std::string str(size, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size, NULL, NULL);   
        return str;
    }

    std::wstring ToUtf16(const std::string& str) {
        if (str.empty()) return L"";
        int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
        std::wstring wstr(size, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
        return wstr;
    }

    std::wstring MaskPassword(const std::wstring& password){
        return std::wstring(password.size(), L'*');
    }

    void SetStatus(AppState& app, const std::wstring& text){
        app.statusMessage = text;
    }

    void SwitchToLogin(AppState& app, const std::wstring& message) {
        app.scene = Scene::Login;
        app.activeField = ActiveField::LoginUsername;
        app.loginPassword.text.clear();
        SetStatus(app, message);
    }

    void SwitchToRegister(AppState& app, const std::wstring& message) {
        app.scene = Scene::Register;
        app.activeField = ActiveField::RegisterUsername;
        SetStatus(app, message);
    }

    int handleLogin(TextField username, TextField password, char msg[]) {
        if (username.text.empty() || password.text.empty()) {
            setbkmode(TRANSPARENT);
            settextcolor(BLACK);
            settextstyle(24, 0, L"Microsoft YaHei UI");
            outtextxy(PANEL_LEFT + 70, PANEL_TOP + 220, L"用户名或密码不能为空");
            return 0;
        }
        FILE* fp = nullptr;
        errno_t err=fopen_s(&fp, FILE_NAME, "r");
        if (err!=0 || fp==nullptr) {
            strcpy_s(msg,100, "用户名和密码不能为空");
            return 0;
        }
        char userBuf[100] = { 0 };
        char pwdBuf[100] = { 0 };
        int found = 0;

     while (fscanf_s(fp, "%s %s", userBuf, (unsigned)_countof(userBuf), pwdBuf, (unsigned)_countof(pwdBuf)) != EOF) {
            std::wstring fileUser(userBuf, userBuf + strlen(userBuf));
            std::wstring filePwd(pwdBuf, pwdBuf + strlen(pwdBuf));
            if (fileUser == username.text && filePwd == password.text) {
                found = 1;
                break;
            }
        }
        fclose(fp);
        if (found) {
            strcpy_s(msg, 100, "登录成功！");
            return 1;
        }
        else {
            strcpy_s(msg, 100, "用户名或密码错误！");
            return 0;
        }
    }

    void handleRegister(TextField& username, TextField&password,char msg[]) {
        if (username.text.empty() ||  password.text.empty()) {
            strcpy_s(msg, 100, "用户名或密码不能为空！");
            return;
        }
        FILE* fp = nullptr;
        errno_t err = fopen_s(&fp, FILE_NAME, "r");
        if (err!=0 || fp==nullptr) {
            err= fopen_s(&fp,FILE_NAME, "w");
            if (err != 0 ||  fp== nullptr) {
                strcpy_s(msg,100, "用户名和密码不能为空");
                return;
            }
            fclose(fp);
            err = fopen_s(&fp, FILE_NAME, "r");
            if (err != 0 || fp == nullptr) {
                strcpy_s(msg, 100, "用户名和密码不能为空");
                return;
            }
        }

        bool userExists = false;
        char userBuf[100] = { 0 };
        char pwdBuf[100] = { 0 };

        while (fscanf_s(fp, "%s %s", userBuf, (unsigned)_countof(userBuf), pwdBuf, (unsigned)_countof(pwdBuf)) != EOF) {
            std::wstring fileUser(userBuf, userBuf + strlen(userBuf));
            if (fileUser == username.text) {
                userExists = true;
                break;
            }
        }
        fclose(fp);
        if (userExists) {
            strcpy_s(msg,100, "用户已存在");
            return;
        }
        err = fopen_s(&fp, FILE_NAME, "a");
        if (err != 0 || fp == nullptr) {
            strcpy_s(msg,100, "密码不正确");
            return;
        }
        std::string userUtf8 = ToUtf8(username.text);
        std::string pwdUtf8 = ToUtf8(password.text);
        fprintf(fp, "%s %s\n", userUtf8.c_str(), pwdUtf8.c_str());
        fclose(fp);

        username.text.clear();
        password.text.clear();
        strcpy_s(msg,100, "登录成功");
}

    TextField* GetActiveTextField(AppState& app) {
        switch (app.activeField) {
        case ActiveField::LoginUsername:
            return &app.loginUsername;
        case ActiveField::LoginPassword:
            return &app.loginPassword;
        case ActiveField::RegisterUsername:
            return &app.registerUsername;
        case ActiveField::RegisterPassword:
            return &app.registerPassword;
        default:
            return nullptr;
        }
    }

    void DrawRoundedCard(const RECT& rect, COLORREF fillColor, COLORREF borderColor, int radius = 14, int borderWidth = 1) {
        setfillcolor(fillColor);
        solidroundrect(rect.left, rect.top, rect.right, rect.bottom, radius, radius);
        setlinecolor(borderColor);
        setlinestyle(PS_SOLID, borderWidth);
        roundrect(rect.left, rect.top, rect.right, rect.bottom, radius, radius);
        setlinestyle(PS_SOLID, 1);
    }

    void DrawButton(const Button& button) {

        setfillcolor(button.color);
        solidroundrect(button.rect.left, button.rect.top, button.rect.right, button.rect.bottom, 12, 12);
        setbkmode(TRANSPARENT); 
        settextcolor(WHITE);
        settextstyle(24, 0, L"Microsoft YaHei UI"); 
        int textW = textwidth(button.label);
        int textH = textheight(button.label);
        int x = button.rect.left + (button.rect.right - button.rect.left - textW) / 2;
        int y = button.rect.top + (button.rect.bottom - button.rect.top - textH) / 2;

        outtextxy(x, y, button.label);
    }
    
    void DrawTextField(const TextField& field, bool active){
        setlinecolor(active ? RGB(49, 121, 245) : RGB(160, 172, 193));
        setlinestyle(PS_SOLID, active ? 3 : 1);
        setfillcolor(RGB(248, 250, 255));
        solidroundrect(field.rect.left, field.rect.top, field.rect.right, field.rect.bottom, 10, 10);
        roundrect(field.rect.left, field.rect.top, field.rect.right, field.rect.bottom, 10, 10);
        
        std::wstring display = field.password ? MaskPassword(field.text) : field.text;
        if (display.empty() && !active){
            settextcolor(RGB(150, 157, 176));
            display = field.placeholder;
        }
        else{
            settextcolor(RGB(34, 42, 60));
        }

        setbkmode(TRANSPARENT);
        settextstyle(24, 0, L"Microsoft YaHei UI");
        outtextxy(field.rect.left + 16, field.rect.top + 10, display.c_str());

        if (active && ((GetTickCount() / 500) % 2 == 0)){
            const std::wstring caretText = field.password ? MaskPassword(field.text) : field.text;
            const int caretX = min(field.rect.left + 18 + textwidth(caretText.c_str()), field.rect.right - 18);
            setlinecolor(RGB(49, 121, 245));
            setlinestyle(PS_SOLID, 2);
            line(caretX, field.rect.top + 12, caretX, field.rect.bottom - 12);
        }
        setlinestyle(PS_SOLID, 1);
    }

    void DrawBackground(){
    }

    void DrawLoginScene(const AppState& app){
        settextcolor(RGB(255, 215, 0));
        settextstyle(45, 0, L"Microsoft YaHei UI");
        outtextxy(PANEL_LEFT + 70, PANEL_TOP + 40, L"登录注册");
        settextcolor(RGB(222, 229, 243));
        settextstyle(21, 0, L"Microsoft YaHei UI");
        outtextxy(PANEL_LEFT + 70, PANEL_TOP + 120, L"用户名");
        outtextxy(PANEL_LEFT + 70, PANEL_TOP + 220, L"密码");

        DrawTextField(app.loginUsername, app.activeField == ActiveField::LoginUsername);
        DrawTextField(app.loginPassword, app.activeField == ActiveField::LoginPassword);
        DrawButton(app.loginButton);
        DrawButton(app.toRegisterButton);
    }

    void DrawRegisterScene(const AppState& app){

        settextcolor(RGB(222, 229, 243));
        settextstyle(21, 0, L"Microsoft YaHei UI");
        outtextxy(PANEL_LEFT + 70, PANEL_TOP + 120, L"新用户名");
        outtextxy(PANEL_LEFT + 70, PANEL_TOP + 220, L"新密码");

        DrawTextField(app.registerUsername, app.activeField == ActiveField::RegisterUsername);
        DrawTextField(app.registerPassword, app.activeField == ActiveField::RegisterPassword);
        DrawButton(app.registerSubmitButton);
        DrawButton(app.registerBackButton);
    }

    void DrawMainScene(const AppState& app){ }

    void DrawAuthBackgroundImage() {
        static IMAGE authBgImage;
        static bool authBgLoaded = LoadAuthBgImage(authBgImage);
        if (authBgLoaded) {
            putimage(0, 0, &authBgImage);
        }
    }

    void DrawTitleScene() {
        drawTitle();
    }

    void Render(AppState& app){       
        cleardevice();
        if (app.scene == Scene::Login || app.scene == Scene::Register) {
            DrawAuthBackgroundImage();
        }
        if (app.scene != Scene::Title) {
            DrawBackground();
        }
        switch (app.scene){
        case Scene::Title:
            DrawTitleScene();
            break;
        case Scene::Login:
            DrawLoginScene(app);
            break;
        case Scene::Register:
            DrawRegisterScene(app);
            break;
        case Scene::Main:
            break;
        }
    }

    void CycleLoginField(AppState& app){
        app.activeField = (app.activeField == ActiveField::LoginUsername) ?
            ActiveField::LoginPassword : ActiveField::LoginUsername;
    }

    void CycleRegisterField(AppState& app){
        app.activeField = (app.activeField == ActiveField::RegisterUsername) ?
            ActiveField::RegisterPassword : ActiveField::RegisterUsername;
    }

    void HandleCharInput(AppState& app, wchar_t ch){
        if (app.scene == Scene::Title) {
            if (ch == 27){
                app.running = false;
            }
            else if (ch == L' ') {
                SwitchToLogin(app, L"请先注册或直接登录。");
            }
            return;
        }
        if (ch == 27){
            app.running = false;
            return;
        }
        if (ch == L'\t'){
            if (app.scene == Scene::Login){
                CycleLoginField(app);
            }
            else if (app.scene == Scene::Register){
                CycleRegisterField(app);
            }
            return;
        }
        if (ch == L'\r') {
            if (app.scene == Scene::Login) {
                char msg[100];
                int ret = handleLogin(app.loginUsername, app.loginPassword, msg);
                app.statusMessage = ToUtf16(msg);
                if (ret == 1) {
                    app.currentUser = app.loginUsername.text;
                    app.scene = Scene::Main;
                }
            }
            else if (app.scene == Scene::Register) {
                char msg[100];
                handleRegister(app.registerUsername, app.registerPassword, msg);
                app.statusMessage = ToUtf16(msg);
            }
            return;
        }
        TextField* field = GetActiveTextField(app);
        if (!field) return;
        if (ch == L'\b') {
            if (!field->text.empty()) field->text.pop_back();
            return;
        }
        if (IsAllowedInput(ch) && field->text.size() < field->maxLength){
            field->text.push_back(ch);
        }
    }

    void HandleLoginMouse(AppState& app, int x, int y){
        if (HitTest(app.loginUsername.rect, x, y)){
            app.activeField = ActiveField::LoginUsername;
        }
        else if (HitTest(app.loginPassword.rect, x, y)){
            app.activeField = ActiveField::LoginPassword;
        }
        else if (HitButton(app.loginButton, x, y)){
            char msg[100];
            int ret = handleLogin(app.loginUsername, app.loginPassword, msg);
            app.statusMessage = ToUtf16(msg);
            if (ret == 1) {
                app.currentUser = app.loginUsername.text;
                app.scene = Scene::Main;
            }
        }
        else if (HitButton(app.toRegisterButton, x, y)){
            app.registerUsername.text.clear();
            app.registerPassword.text.clear();
            SwitchToRegister(app, L"请填写新账号信息。");
        }
        else{
            app.activeField = ActiveField::None;
        }
    }

    void HandleRegisterMouse(AppState& app, int x, int y){
        if (HitTest(app.registerUsername.rect, x, y)){
            app.activeField = ActiveField::RegisterUsername;
        }
        else if (HitTest(app.registerPassword.rect, x, y)){
            app.activeField = ActiveField::RegisterPassword;
        }
        else if (HitButton(app.registerSubmitButton, x, y)){
            char msg[100];
            handleRegister(app.registerUsername, app.registerPassword, msg);
            app.statusMessage = ToUtf16(msg);
        }
        else if (HitButton(app.registerBackButton, x, y)){
            SwitchToLogin(app, L"已返回登录界面。");
        }
        else{
            app.activeField = ActiveField::None;
        }
    }

    void HandleMainMouse(AppState& app, int x, int y) { 
        if (HitButton(app.logoutButton, x, y)) {
            app.currentUser.clear();
            SwitchToLogin(app, L"已退出登录。");
        }
        else if (HitButton(app.exitButton, x, y)) {
            app.running = false;
        }
    }

    void HandleMouseClick(AppState& app, int x, int y) {
        switch (app.scene) {
        case Scene::Title:
            break;
        case Scene::Login:
            HandleLoginMouse(app, x, y);
            break;
        case Scene::Register:
            HandleRegisterMouse(app, x, y);
            break;
        case Scene::Main:
            HandleMainMouse(app, x, y);
            break;
        }
    }
}




void LoadAccounts(AppState& app){
    app.accounts.clear();
    std::ifstream input((USERS_FILE));
    if (!input.is_open()){
        return;
    }
    std::string line;
    while (std::getline(input, line)){
        const size_t split = line.find('\t');
        if (split == std::string::npos){
            continue;
        }
        Account account;
        account.username = ToUtf16(line.substr(0, split));
        account.password = ToUtf16(line.substr(split + 1));
        if (!account.username.empty()){
            app.accounts.push_back(account);
        }
    }
}

int main() {
    AppState app;
    LoadAccounts(app); 
    initgraph(WINDOW_WIDTH, WINDOW_HEIGHT);
    setbkcolor(RGB(232, 239, 249));
    BeginBatchDraw();
    while (app.running) {
        if (app.scene == Scene::Main) {
            run_checkerboard();
            app.currentUser.clear();
            app.running = false;
            continue;
        }
        Render(app);
        FlushBatchDraw();
        ExMessage msg = {};
        while (peekmessage(&msg, EX_MOUSE | EX_CHAR)) {
            if (msg.message == WM_LBUTTONDOWN) {
                HandleMouseClick(app, msg.x, msg.y);
            }
            else if (msg.message == WM_CHAR) {
                HandleCharInput(app, static_cast<wchar_t>(msg.ch));
            }
        }
        Sleep(16);
    }
    EndBatchDraw();
    closegraph();
    return 0;
}

void drawTitle() {
    static IMAGE titleImage;
    static bool titleImageLoaded = LoadTitleImage(titleImage);

    if (titleImageLoaded) {
        putimage(0, 0, &titleImage);
    }
}
