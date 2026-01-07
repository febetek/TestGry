#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <optional>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>
#include <random>

using namespace std;
using namespace sf;

// --- ENUMY ---

enum WeaponId {
    WHIP, MAGIC_WAND, KNIFE, AXE, BOOMERANG,
    BIBLE, FIRE_WAND, GARLIC, HOLY_WATER, RUNETRACER, LIGHTNING
};

enum PassiveId {
    HOLLOW_HEART,   // 1. Max HP
    EMPTY_TOME,     // 2. Szybkostrzelnosc
    BRACER,         // 3. Szybkosc pocisku
    CANDLE,         // 4. Obszar
    CLOVER,         // 5. Szczescie
    SPELLBINDER,    // 6. Czas trwania
    SPINACH,        // 7. Obrazenia
    PUMMAROLA,      // 8. Regeneracja
    ATTRACTORB,     // 9. Magnes
    ARMOR,          // 10. Pancerz
    DUPLICATOR      // 11. Ilosc pociskow (Max lvl 3)
};

// --- STRUKTURY DANYCH ---

struct WeaponDef {
    WeaponId id;
    std::string name;
    std::string description;
    sf::Color color;
    float baseCooldown;
    float duration;
    float speed;
    float damage;
};

struct PassiveDef {
    PassiveId id;
    std::string name;
    std::string description;
    sf::Color color;
    int maxLevel; // 7 dla wiÍkszoúci, 3 dla duplikatora
};

struct ActiveWeapon {
    WeaponDef def;
    int level = 1;
    float cooldownTimer = 0.0f;
};

struct ActivePassive {
    PassiveDef def;
    int level = 1;
};

// Statystyki gracza przeliczane na biezaco
struct PlayerStats {
    int maxHp = 5;
    float moveSpeed = 300.0f;
    float might = 1.0f;         // Obrazenia (Spinach)
    float area = 1.0f;          // Obszar (Candle)
    float speed = 1.0f;         // Predkosc pocisku (Bracer)
    float duration = 1.0f;      // Czas trwania (Spellbinder)
    float cooldown = 1.0f;      // Redukcja cooldownu (Tome)
    int amount = 0;             // Dodatkowe pociski (Duplicator)
    float magnet = 100.0f;      // Zasieg (Attractorb)
    float luck = 1.0f;          // (Clover)
    float regen = 0.0f;         // HP na sekunde (Pummarola)
    int armor = 0;              // Redukcja obrazen (Armor)
};

struct Enemy {
    sf::CircleShape shape;
    sf::Vector2f velocity;
    int id = 0;
    float hp = 1.0f;
    float maxHp = 1.0f;
    bool isBoss = false;
};

struct Projectile {
    WeaponId wId;
    sf::CircleShape shape;
    sf::RectangleShape boxShape;
    sf::Vector2f velocity;
    sf::Vector2f startPos;
    float lifeTime = 0.0f;
    float maxLifeTime = 0.0f;
    bool returning = false;
    int pierceLeft = 0;
    std::vector<int> hitEnemies;
    bool isRunetracer = false;
    float damage = 1.0f; // Obraøenia pocisku
};

struct DamageZone {
    WeaponId wId;
    sf::CircleShape shape;
    float lifeTime = 0.0f;
    float maxLifeTime = 0.0f;
    float tickTimer = 0.0f;
    float damage = 1.0f;
};

struct ExpOrb {
    sf::CircleShape shape;
    sf::Vector2f velocity;
    int value = 0;
    bool isOnGround = false;
};

// --- STRUKTURY UI ---

struct WeaponButton {
    sf::RectangleShape shape;
    sf::Text text;
    WeaponId id = WHIP;
    WeaponButton(const sf::Font& font) : text(font) {}
};

struct UpgradeCard {
    sf::RectangleShape shape;
    sf::Text mainText;
    sf::Text subText;

    // Typ karty: 0 = BroÒ, 1 = Pasywka
    int type = 0;
    int index = 0; // Indeks w bazie danych
    bool isNew = false;

    UpgradeCard(const sf::Font& font) : mainText(font), subText(font) {}
};

// --- BAZA DANYCH ---

std::vector<WeaponDef> weaponDB = {
    {WHIP, "Szalik AGH", "Atakuje poziomo", sf::Color(139, 69, 19), 1.0f, 0.2f, 0.f, 10.f},
    {MAGIC_WAND, "Komputer", "Strzela w najblizszego", sf::Color(0, 255, 255), 1.0f, 3.0f, 600.f, 5.f},
    {KNIFE, "Olowek", "Leci w kierunku patrzenia", sf::Color(200, 200, 200), 0.5f, 2.0f, 1000.f, 5.f},
    {AXE, "Krzeslo", "Leci lobem w gore", sf::Color(100, 100, 100), 1.2f, 2.0f, 500.f, 15.f},
    {BOOMERANG, "Podrecznik do analizy", "Leci i wraca", sf::Color(0, 0, 255), 1.3f, 2.0f, 700.f, 10.f},
    {BIBLE, "Piwo AGH-owskie", "Orbituje wokol ciebie", sf::Color(255, 255, 0), 3.0f, 3.0f, 3.0f, 5.f},
    {FIRE_WAND, "Kawalki stali", "Potezne pociski", sf::Color(255, 69, 0), 1.5f, 4.0f, 300.f, 20.f},
    {GARLIC, "Wiedza AGH", "Aura raniaca wrogow", sf::Color(255, 200, 200), 0.15f, 9999.f, 0.f, 3.f},
    {HOLY_WATER, "Fiolka z laboratorium", "Tworzy plame", sf::Color(0, 100, 255), 3.0f, 3.0f, 400.f, 5.f},
    {RUNETRACER, "Dlugopis", "Przebija i skacze", sf::Color(255, 0, 255), 1.5f, 3.0f, 800.f, 8.f},
    {LIGHTNING, "Studencki czwartek", "Uderza z nieba", sf::Color(200, 200, 0), 1.2f, 0.15f, 0.f, 20.f}
};

std::vector<PassiveDef> passiveDB = {
    {HOLLOW_HEART, "Serce Witalnosci", "Zwieksza Max HP (+20%)", sf::Color(200, 0, 0), 7},
    {EMPTY_TOME, "Energetyk", "Szybkostrzelnosc (-8% cooldown)", sf::Color(200, 200, 0), 7},
    {BRACER, "Moc rzutu", "Szybkosc pocisku (+10%)", sf::Color(100, 100, 255), 7},
    {CANDLE, "Wieksze pociski", "Obszar pocisku (+10%)", sf::Color(255, 150, 0), 7},
    {CLOVER, "Koniczyna", "Szczescie (+10%)", sf::Color(0, 255, 0), 7},
    {SPELLBINDER, "Sprawdzenie Obecnosci", "Czas trwania (+10%)", sf::Color(100, 0, 255), 7},
    {SPINACH, "Sila", "Obrazenia (+10%)", sf::Color(0, 150, 0), 7},
    {PUMMAROLA, "Serce Regeneracji", "Regeneracja HP (+0.2/s)", sf::Color(255, 100, 100), 7},
    {ATTRACTORB, "Magnes", "Zasieg zbierania (+30%)", sf::Color(0, 0, 255), 7},
    {ARMOR, "Pancerz", "Redukcja obrazen (+1)", sf::Color(150, 150, 150), 7},
    {DUPLICATOR, "Duplikator", "Ilosc pociskow (+1)", sf::Color(0, 255, 255), 3}
};



// --- FUNKCJE POMOCNICZE ---

float vectorLength(sf::Vector2f v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

sf::Vector2f normalize(sf::Vector2f v) {
    float len = vectorLength(v);
    if (len != 0) return v / len;
    return v;
}

void centerText(sf::Text& text, float x, float y) {
    sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin({ bounds.size.x / 2.f, bounds.size.y / 2.f });
    text.setPosition({ x, y });
}

// Funkcja przeliczajπca statystyki
PlayerStats recalculateStats(const std::vector<ActivePassive>& passives) {
    PlayerStats stats;
    // Bazowe
    stats.maxHp = 10;
    stats.moveSpeed = 300.f;
    stats.magnet = 100.f;

    for (const auto& p : passives) {
        float bonus = (float)p.level;
        switch (p.def.id) {
        case HOLLOW_HEART: stats.maxHp += (int)(bonus * 2); break; // +2 HP per level
        case EMPTY_TOME: stats.cooldown -= (bonus * 0.08f); break; // -8% per level
        case BRACER: stats.speed += (bonus * 0.1f); break;
        case CANDLE: stats.area += (bonus * 0.1f); break;
        case CLOVER: stats.luck += (bonus * 0.1f); break;
        case SPELLBINDER: stats.duration += (bonus * 0.1f); break;
        case SPINACH: stats.might += (bonus * 0.1f); break;
        case PUMMAROLA: stats.regen += (bonus * 0.2f); break;
        case ATTRACTORB: stats.magnet += (bonus * 50.f); break;
        case ARMOR: stats.armor += (int)bonus; break;
        case DUPLICATOR: stats.amount += (int)bonus; break;
        }
    }
    // Limit cooldownu zeby nie bylo 0
    if (stats.cooldown < 0.1f) stats.cooldown = 0.1f;
    return stats;
}

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    unsigned int windowWidth = 1920;
    unsigned int windowHeight = 1080;

    sf::RenderWindow window(sf::VideoMode({ windowWidth, windowHeight }), "Projekt Podstawy Informatyki");
    window.setFramerateLimit(140);

    bool isFullscreen = false;

    sf::Texture bgTexture;
    if (!bgTexture.loadFromFile("sprites/background.png")) return -1;
    sf::Sprite background(bgTexture);
    float mapWidth = (float)bgTexture.getSize().x;

    sf::Font font;
    if (!font.openFromFile("czcionka/arial.ttf")) return -1;                                    // wczytywanie czcionki

    // --- AUDIO (INIT) - SFML 3.0 FIX ---                                             //********************************************
    sf::Music bgMusic;
    if (!bgMusic.openFromFile("dzwieki/music.ogg")) {
        std::cout << "Brak pliku music.ogg!" << std::endl;
    }
    float musicScale = 30.0f;
    bgMusic.setLooping(true); // <--- ZMIANA: setLooping zamiast setLoop                // ustawienia muzyki
    bgMusic.setVolume(musicScale);

    sf::SoundBuffer jumpBuffer;
    if (!jumpBuffer.loadFromFile("dzwieki/jump_sound.wav")) {
        std::cout << "Brak pliku jump_sound.wav!" << std::endl;
    }

    // SFML 3.0: Tworzymy sf::Sound podajπc bufor w konstruktorze,
    // aby uniknπÊ b≥Ídu o braku domyúlnego konstruktora.

    float sfxScale = 50.0f;
    sf::Sound jumpSound(jumpBuffer);
    jumpSound.setVolume(sfxScale);                                                      // ustawienia g≥oúnoúci skoku

    sf::SoundBuffer clickBuffer;
    if (!clickBuffer.loadFromFile("dzwieki/click.wav")) {
        std::cout << "Brak pliku click.wav!" << std::endl;
    }
    sf::Sound clickSound(clickBuffer);
    clickSound.setVolume(sfxScale);                                                     // ustawienia g≥oúnoúci klikniÍcia

    sf::SoundBuffer gameOverBuffer;
    if (!gameOverBuffer.loadFromFile("dzwieki/game_over.ogg")) {
        std::cout << "Brak pliku game_over.ogg!" << std::endl;
    }
    sf::Sound gameOverSound(gameOverBuffer); // Konstruktor SFML 3.0                                                        // ustawienia düwiÍku gameover
    gameOverSound.setVolume(sfxScale * 1.5f);
    // -----------------------------------


    // --- UI SETUP ---
    sf::RectangleShape uiBar({ (float)windowWidth, 100.f }); uiBar.setFillColor(sf::Color(0, 0, 0, 150));
    sf::RectangleShape overlay({ (float)windowWidth, (float)windowHeight }); overlay.setFillColor(sf::Color(0, 0, 0, 200));

    sf::Text hpText(font); hpText.setCharacterSize(30); hpText.setPosition({ 20.f, 10.f });
    sf::Text lvlText(font); lvlText.setCharacterSize(25); lvlText.setFillColor(sf::Color::Cyan); lvlText.setPosition({ 20.f, 50.f });
    sf::Text timeText(font); timeText.setCharacterSize(40);

    sf::Text weaponsListText(font); weaponsListText.setCharacterSize(18); weaponsListText.setPosition({ windowWidth - 300.f, 10.f });
    sf::Text passivesListText(font); passivesListText.setCharacterSize(18); passivesListText.setPosition({ windowWidth - 150.f, 10.f });

    sf::Text title(font); title.setString("WYBIERZ BRON STARTOWA"); title.setCharacterSize(60); centerText(title, windowWidth / 2.f, 100.f);

    sf::Text pauseTitle(font);                                                                                      // font to czcionka
    pauseTitle.setString("PAUZA");
    pauseTitle.setCharacterSize(80);
    pauseTitle.setFillColor(sf::Color::Yellow);
    centerText(pauseTitle, windowWidth / 2.f, windowHeight / 2.f - 200.f);

    sf::Text resumeInfo(font);
    resumeInfo.setString(L"ESC - WznÛw");
    resumeInfo.setCharacterSize(30);
    resumeInfo.setFillColor(sf::Color::Green);
    centerText(resumeInfo, windowWidth / 2.f, windowHeight / 2.f + 50.f);

    sf::Text pauseRestart(font);
    pauseRestart.setString("RESTART GAME");
    pauseRestart.setCharacterSize(40);
    pauseRestart.setFillColor(sf::Color::White);
    centerText(pauseRestart, windowWidth / 2.f, windowHeight / 2.f + 120.f);

    sf::Text gameOverTitle(font), restartButton(font);
    gameOverTitle.setString("GAME OVER"); gameOverTitle.setCharacterSize(100); gameOverTitle.setFillColor(sf::Color::Red); centerText(gameOverTitle, windowWidth / 2.f, windowHeight / 2.f - 100.f);
    restartButton.setString("RESTART"); restartButton.setCharacterSize(60); restartButton.setFillColor(sf::Color::White); centerText(restartButton, windowWidth / 2.f, windowHeight / 2.f + 100.f);

    // --- SETUP PRZYCISK”W WYBORU BRONI ---
    std::vector<WeaponButton> weaponButtons;
    weaponButtons.reserve(weaponDB.size());

    float startX = 200.f, startY = 250.f, gapX = 350.f, gapY = 150.f;
    int col = 0, row = 0;

    for (const auto& w : weaponDB) {
        weaponButtons.emplace_back(font);
        WeaponButton& btn = weaponButtons.back();
        btn.id = w.id;
        btn.shape.setSize({ 300.f, 100.f });
        btn.shape.setFillColor(sf::Color(50, 50, 50));
        btn.shape.setOutlineColor(w.color);
        btn.shape.setOutlineThickness(2.f);
        btn.shape.setPosition({ startX + col * gapX, startY + row * gapY });
        btn.text.setString(w.name);
        btn.text.setCharacterSize(24);
        btn.text.setFillColor(w.color);
        centerText(btn.text, btn.shape.getPosition().x + 150.f, btn.shape.getPosition().y + 50.f);
        col++; if (col > 3) { col = 0; row++; }
    }

    // --- SETUP MENU ---
    sf::Text menuTitle(font);                                                              // zmienna SFML typu Text 
    menuTitle.setString("                   PROJEKT \n      PODSTAWY INFORMATYKI");
    menuTitle.setCharacterSize(80);
    menuTitle.setFillColor(sf::Color::Red);
    menuTitle.setOutlineColor(sf::Color::White);
    menuTitle.setOutlineThickness(2.f);
    centerText(menuTitle, windowWidth / 2.f, windowHeight / 2.f - 200.f);

    sf::Text menuStart(font);
    menuStart.setString("GRAJ");
    menuStart.setCharacterSize(50);
    menuStart.setFillColor(sf::Color::White);
    menuStart.setOutlineColor(sf::Color::White);
    menuStart.setOutlineThickness(1.5f);
    centerText(menuStart, windowWidth / 2.f, windowHeight / 2.f - 0.f);

    sf::Text menuExit(font);
    menuExit.setString(L"WYJåCIE");
    menuExit.setCharacterSize(50);
    menuExit.setFillColor(sf::Color::White);
    menuExit.setOutlineColor(sf::Color::White);
    menuExit.setOutlineThickness(1.5f);
    centerText(menuExit, windowWidth / 2.f, windowHeight / 2.f + 200.f);

    // --- Napisy - G≥Ûwne ustawienia --- 

    Text menuSettings(font);
    menuSettings.setString("USTAWIENIA");
    menuSettings.setCharacterSize(50);
    menuSettings.setFillColor(Color::White);
    menuSettings.setOutlineColor(Color::White);
    menuSettings.setOutlineThickness(1.5f);
    centerText(menuSettings, windowWidth / 2.0f, windowHeight / 2.0f + 100.0f);

    Text settingsTitle(font);
    settingsTitle.setString("               USTAWIENIA              ");
    settingsTitle.setCharacterSize(80);
    settingsTitle.setFillColor(Color::Red);
    settingsTitle.setOutlineColor(Color::White);
    settingsTitle.setOutlineThickness(2.f);
    centerText(settingsTitle, windowWidth / 2.0f, windowHeight / 2.0f - 200.0f);

    Text graphicsSettings(font);
    graphicsSettings.setString("GRAFIKA");
    graphicsSettings.setCharacterSize(50);
    graphicsSettings.setFillColor(Color::White);
    graphicsSettings.setOutlineColor(Color::White);
    graphicsSettings.setOutlineThickness(1.5f);
    centerText(graphicsSettings, windowWidth / 2.0f, windowHeight / 2.0f - 50.0f);

    Text keyBindings(font);
    keyBindings.setString("STEROWANIE");
    keyBindings.setCharacterSize(50);
    keyBindings.setFillColor(Color::White);
    keyBindings.setOutlineColor(Color::White);
    keyBindings.setOutlineThickness(1.5f);
    centerText(keyBindings, windowWidth / 2.0f, windowHeight / 2.0f + 50.0f);

    Text volumeSettings(font);
    volumeSettings.setString(L"DèWI K");
    volumeSettings.setCharacterSize(50);
    volumeSettings.setFillColor(Color::White);
    volumeSettings.setOutlineColor(Color::White);
    volumeSettings.setOutlineThickness(1.5f);
    centerText(volumeSettings, windowWidth / 2.0f, windowHeight / 2.0f + 150.0f);

    Text SettingsExit(font);
    SettingsExit.setString(L"WYJåCIE");
    SettingsExit.setCharacterSize(50);
    SettingsExit.setFillColor(Color::White);
    SettingsExit.setOutlineColor(Color::White);
    SettingsExit.setOutlineThickness(1.5f);
    centerText(SettingsExit, windowWidth / 2.0f, windowHeight / 2.0f + 250.0f);

    // --- GRAFIKA ---

    Text graphicsSettingsTitle(font);
    graphicsSettingsTitle.setString(L"               GRAFIKA              ");
    graphicsSettingsTitle.setCharacterSize(80);
    graphicsSettingsTitle.setFillColor(Color::Red);
    graphicsSettingsTitle.setOutlineColor(Color::White);
    graphicsSettingsTitle.setOutlineThickness(2.f);
    centerText(graphicsSettingsTitle, windowWidth / 2.f, windowHeight / 2.f - 200.0f);

    Text fullscreenSettings(font);
    fullscreenSettings.setString(L"PE£NY EKRAN");
    fullscreenSettings.setCharacterSize(50);
    fullscreenSettings.setFillColor(Color::White);
    fullscreenSettings.setOutlineColor(Color::White);
    fullscreenSettings.setOutlineThickness(1.5f);
    centerText(fullscreenSettings, windowWidth / 2.0f, windowHeight / 2.0f - 50.0f);

    Text windowedSettings(font);
    windowedSettings.setString(L"TRYB OKIENKOWY");
    windowedSettings.setCharacterSize(50);
    windowedSettings.setFillColor(Color::White);
    windowedSettings.setOutlineColor(Color::White);
    windowedSettings.setOutlineThickness(1.5f);
    centerText(windowedSettings, windowWidth / 2.0f, windowHeight / 2.0f + 50.0f);

    Text graphicsSettingsExit(font);
    graphicsSettingsExit.setString(L"WYJåCIE");
    graphicsSettingsExit.setCharacterSize(50);
    graphicsSettingsExit.setFillColor(Color::White);
    graphicsSettingsExit.setOutlineColor(Color::White);
    graphicsSettingsExit.setOutlineThickness(1.5f);
    centerText(graphicsSettingsExit, windowWidth / 2.0f, windowHeight / 2.0f + 150.0f);

    // --- STEROWANIE ---

    Text keyBindingsTitle(font);
    keyBindingsTitle.setString(L"               STEROWANIE              ");
    keyBindingsTitle.setCharacterSize(80);
    keyBindingsTitle.setFillColor(Color::Red);
    keyBindingsTitle.setOutlineColor(Color::White);
    keyBindingsTitle.setOutlineThickness(2.f);
    centerText(keyBindingsTitle, windowWidth / 2.f, windowHeight / 2.f - 400.0f);

    // --- Sterowanie (mechanicznie) SKOK ---
    Keyboard::Scancode keyJump1 = Keyboard::Scancode::Up;
    Keyboard::Scancode keyJump2 = Keyboard::Scancode::W;
    Keyboard::Scancode keyJump3 = Keyboard::Scancode::Space;

    // --- Sterowanie (mechanicznie) LEWO ---
    Keyboard::Scancode keyLeft1 = Keyboard::Scancode::Left;
    Keyboard::Scancode keyLeft2 = Keyboard::Scancode::A;
    Keyboard::Scancode keyLeft3 = Keyboard::Scancode::Unknown;

    // --- Sterowanie (mechanicznie) BIEG ---
    Keyboard::Scancode keyRun1 = Keyboard::Scancode::Down;
    Keyboard::Scancode keyRun2 = Keyboard::Scancode::S;
    Keyboard::Scancode keyRun3 = Keyboard::Scancode::LShift;

    // --- Sterowanie (mechanicznie) PRAWO ---
    Keyboard::Scancode keyRight1 = Keyboard::Scancode::Right;
    Keyboard::Scancode keyRight2 = Keyboard::Scancode::D;
    Keyboard::Scancode keyRight3 = Keyboard::Scancode::Unknown;

    String JumpName1 = Keyboard::getDescription(keyJump1);
    Text Jump1(font);
    Jump1.setString(L"SKOK (" + JumpName1 + ")");
    Jump1.setCharacterSize(50);
    Jump1.setFillColor(Color::White);
    Jump1.setOutlineColor(Color::White);
    Jump1.setOutlineThickness(1.5f);
    centerText(Jump1, windowWidth / 2.0f - 500.0f, windowHeight / 2.0f - 250.0f);

    String JumpName2 = Keyboard::getDescription(keyJump2);
    Text Jump2(font);
    Jump2.setString(L"SKOK (" + JumpName2 + ")");
    Jump2.setCharacterSize(50);
    Jump2.setFillColor(Color::White);
    Jump2.setOutlineColor(Color::White);
    Jump2.setOutlineThickness(1.5f);
    centerText(Jump2, windowWidth / 2.0f, windowHeight / 2.0f - 250.0f);

    String JumpName3 = Keyboard::getDescription(keyJump3);
    Text Jump3(font);
    Jump3.setString(L"SKOK (" + JumpName3 + ")");
    Jump3.setCharacterSize(50);
    Jump3.setFillColor(Color::White);
    Jump3.setOutlineColor(Color::White);
    Jump3.setOutlineThickness(1.5f);
    centerText(Jump3, windowWidth / 2.0f + 500.0f, windowHeight / 2.0f - 250.0f);

    String LeftName1 = Keyboard::getDescription(keyLeft1);
    Text Left1(font);
    Left1.setString(L"LEWO (" + LeftName1 + ")");
    Left1.setCharacterSize(50);
    Left1.setFillColor(Color::White);
    Left1.setOutlineColor(Color::White);
    Left1.setOutlineThickness(1.5f);
    centerText(Left1, windowWidth / 2.0f - 500.0f, windowHeight / 2.0f - 150.0f);

    String LeftName2 = Keyboard::getDescription(keyLeft2);
    Text Left2(font);
    Left2.setString(L"LEWO (" + LeftName2 + ")");
    Left2.setCharacterSize(50);
    Left2.setFillColor(Color::White);
    Left2.setOutlineColor(Color::White);
    Left2.setOutlineThickness(1.5f);
    centerText(Left2, windowWidth / 2.0f, windowHeight / 2.0f - 150.0f);

    String LeftName3 = Keyboard::getDescription(keyLeft3);
    Text Left3(font);
    Left3.setString(L"LEWO (" + LeftName3 + ")");
    Left3.setCharacterSize(50);
    Left3.setFillColor(Color::White);
    Left3.setOutlineColor(Color::White);
    Left3.setOutlineThickness(1.5f);
    centerText(Left3, windowWidth / 2.0f + 500.0f, windowHeight / 2.0f - 150.0f);

    String RunName1 = Keyboard::getDescription(keyRun1);
    Text Run1(font);
    Run1.setString(L"BIEG (" + RunName1 + ")");
    Run1.setCharacterSize(50);
    Run1.setFillColor(Color::White);
    Run1.setOutlineColor(Color::White);
    Run1.setOutlineThickness(1.5f);
    centerText(Run1, windowWidth / 2.0f - 500.0f, windowHeight / 2.0f - 50.0f);

    String RunName2 = Keyboard::getDescription(keyRun2);
    Text Run2(font);
    Run2.setString(L"BIEG (" + RunName2 + ")");
    Run2.setCharacterSize(50);
    Run2.setFillColor(Color::White);
    Run2.setOutlineColor(Color::White);
    Run2.setOutlineThickness(1.5f);
    centerText(Run2, windowWidth / 2.0f, windowHeight / 2.0f - 50.0f);

    String RunName3 = Keyboard::getDescription(keyRun3);
    Text Run3(font);
    Run3.setString(L"BIEG (" + RunName3 + ")");
    Run3.setCharacterSize(50);
    Run3.setFillColor(Color::White);
    Run3.setOutlineColor(Color::White);
    Run3.setOutlineThickness(1.5f);
    centerText(Run3, windowWidth / 2.0f + 500.0f, windowHeight / 2.0f - 50.0f);

    String RightName1 = Keyboard::getDescription(keyRight1);
    Text Right1(font);
    Right1.setString(L"PRAWO (" + RightName1 + ")");
    Right1.setCharacterSize(50);
    Right1.setFillColor(Color::White);
    Right1.setOutlineColor(Color::White);
    Right1.setOutlineThickness(1.5f);
    centerText(Right1, windowWidth / 2.0f - 500.0f, windowHeight / 2.0f + 50.0f);

    String RightName2 = Keyboard::getDescription(keyRight2);
    Text Right2(font);
    Right2.setString(L"PRAWO (" + RightName2 + ")");
    Right2.setCharacterSize(50);
    Right2.setFillColor(Color::White);
    Right2.setOutlineColor(Color::White);
    Right2.setOutlineThickness(1.5f);
    centerText(Right2, windowWidth / 2.0f, windowHeight / 2.0f + 50.0f);

    String RightName3 = Keyboard::getDescription(keyRight3);
    Text Right3(font);
    Right3.setString(L"PRAWO (" + RightName3 + ")");
    Right3.setCharacterSize(50);
    Right3.setFillColor(Color::White);
    Right3.setOutlineColor(Color::White);
    Right3.setOutlineThickness(1.5f);
    centerText(Right3, windowWidth / 2.0f + 500.0f, windowHeight / 2.0f + 50.0f);

    Text keyBindingsReset(font);
    keyBindingsReset.setString(L"ZRESETUJ");
    keyBindingsReset.setCharacterSize(50);
    keyBindingsReset.setFillColor(Color::White);
    keyBindingsReset.setOutlineColor(Color::White);
    keyBindingsReset.setOutlineThickness(1.5f);
    centerText(keyBindingsReset, windowWidth / 2.0f, windowHeight / 2.0f + 150.0f);

    Text keyBindingsExit(font);
    keyBindingsExit.setString(L"WYJåCIE");
    keyBindingsExit.setCharacterSize(50);
    keyBindingsExit.setFillColor(Color::White);
    keyBindingsExit.setOutlineColor(Color::White);
    keyBindingsExit.setOutlineThickness(1.5f);
    centerText(keyBindingsExit, windowWidth / 2.0f, windowHeight / 2.0f + 250.0f);

    Text ControlsTitle(font);
    ControlsTitle.setString(L"          WCIåNIJ KLAWISZ, KT”RY CHCESZ PRZYPISA∆,\n                  DELETE, ABY USUN•∆ PRZYPISANIE\n                                            LUB\n                                 ESC, ABY WYJå∆");
    ControlsTitle.setCharacterSize(50);
    ControlsTitle.setFillColor(Color(0xFFFFFFFF));
    ControlsTitle.setOutlineColor(Color(0x000000FF));
    ControlsTitle.setOutlineThickness(2.f);
    centerText(ControlsTitle, windowWidth / 2.f, windowHeight / 2.f - 400.0f);

    // --- Ustawienia g≥oúnoúci ---   

    Text volumeSettingsTitle(font);
    volumeSettingsTitle.setString(L"               DèWI K              ");
    volumeSettingsTitle.setCharacterSize(80);
    volumeSettingsTitle.setFillColor(Color::Red);
    volumeSettingsTitle.setOutlineColor(Color::White);
    volumeSettingsTitle.setOutlineThickness(2.f);
    centerText(volumeSettingsTitle, windowWidth / 2.f, windowHeight / 2.f - 400.0f);

    // --- Napisy - Muzyka ---

    Text musicSettings(font);
    musicSettings.setString(L"MUZYKA");
    musicSettings.setCharacterSize(50);
    musicSettings.setFillColor(Color(0xFFFFFFFF));
    musicSettings.setOutlineColor(Color(0x000000FF));
    musicSettings.setOutlineThickness(1.5f);
    centerText(musicSettings, windowWidth / 2.0f, windowHeight / 2.0f - 250.0f);

    // --- Graficzne ustawianie suwaka muzyki --- 

    RectangleShape musicSliderBg({ 400.f, 10.f });
    musicSliderBg.setFillColor(Color(0xD3D3D3FF));
    musicSliderBg.setOrigin({ 200.f, 5.f });
    musicSliderBg.setOutlineColor(Color(0x000000FF));
    musicSliderBg.setOutlineThickness(1.5f);
    musicSliderBg.setPosition({ (float)windowWidth / 2.0f, windowHeight / 2.0f - 150.0f });

    RectangleShape musicSliderButton({ 20.f, 30.f });
    musicSliderButton.setFillColor(sf::Color::Blue);
    musicSliderButton.setOutlineColor(Color(0x000000FF));
    musicSliderButton.setOutlineThickness(1.5f);
    musicSliderButton.setOrigin({ 10.f, 15.f });

    // VolumeMusicScale - maksymalna moøliwa g≥oúnoúÊ muzyki
    float volumeMusicScale = (musicScale + 70.0f);

    float centerX = (float)windowWidth / 2.0f;
    float musicY = windowHeight / 2.0f - 150.0f;
    float SliderMusicStart = centerX - 200.f;
    float musicStartX = SliderMusicStart + (bgMusic.getVolume() / volumeMusicScale) * 400.f;
    musicSliderButton.setPosition({ musicStartX, musicY });

    // --- Przyciski Start i Stop - Muzyka ---

    Text musicStart(font);
    musicStart.setString(L"|>");
    musicStart.setCharacterSize(50);
    musicStart.setFillColor(Color(0xFFFFFFFF));
    musicStart.setOutlineColor(Color(0x000000FF));
    musicStart.setOutlineThickness(1.5f);
    centerText(musicStart, windowWidth / 2.0f - 50.0f, windowHeight / 2.0f - 100.0f);

    Text musicStop(font);
    musicStop.setString(L"||");
    musicStop.setCharacterSize(50);
    musicStop.setFillColor(Color(0xFFFFFFFF));
    musicStop.setOutlineColor(Color(0x000000FF));
    musicStop.setOutlineThickness(1.5f);
    centerText(musicStop, windowWidth / 2.0f + 50.0f, windowHeight / 2.0f - 100.0f);

    // --- Napisy - SFX ---

    Text sfxSettings(font);
    sfxSettings.setString(L"SFX");
    sfxSettings.setCharacterSize(50);
    sfxSettings.setFillColor(Color(0xFFFFFFFF));
    sfxSettings.setOutlineColor(Color(0x000000FF));
    sfxSettings.setOutlineThickness(1.5f);
    centerText(sfxSettings, windowWidth / 2.0f, windowHeight / 2.0f + 0.0f);

    RectangleShape sfxSliderBg({ 400.f, 10.f });
    sfxSliderBg.setFillColor(Color(0xD3D3D3FF));
    sfxSliderBg.setOrigin({ 200.f, 5.f });
    sfxSliderBg.setOutlineColor(Color(0x000000FF));
    sfxSliderBg.setOutlineThickness(1.5f);
    sfxSliderBg.setPosition({ (float)windowWidth / 2.0f, windowHeight / 2.0f + 100.0f });

    RectangleShape sfxSliderButton({ 20.f, 30.f });
    sfxSliderButton.setFillColor(sf::Color::Blue);
    sfxSliderButton.setOutlineColor(Color(0x000000FF));
    sfxSliderButton.setOutlineThickness(1.5f);
    sfxSliderButton.setOrigin({ 10.f, 15.f });

    // VolumeSfxScale - maksymalna moøliwa g≥oúnoúÊ SFX
    float volumeSfxScale = (sfxScale + 50.0f);

    centerX = (float)windowWidth / 2.0f;
    float sfxY = windowHeight / 2.0f + 100.0f;
    float SliderSfxStart = centerX - 200.f;
    float sfxStartX = SliderSfxStart + (jumpSound.getVolume() / volumeSfxScale) * 400.f;
    sfxSliderButton.setPosition({ sfxStartX, sfxY });

    // --- Przyciski Start i Stop - SFX ---

    Text sfxStart(font);
    sfxStart.setString(L"|>");
    sfxStart.setCharacterSize(50);
    sfxStart.setFillColor(Color(0xFFFFFFFF));
    sfxStart.setOutlineColor(Color(0x000000FF));
    sfxStart.setOutlineThickness(1.5f);
    centerText(sfxStart, windowWidth / 2.0f - 50.0f, windowHeight / 2.0f + 150.0f);

    Text sfxStop(font);
    sfxStop.setString(L"||");
    sfxStop.setCharacterSize(50);
    sfxStop.setFillColor(Color(0xFFFFFFFF));
    sfxStop.setOutlineColor(Color(0x000000FF));
    sfxStop.setOutlineThickness(1.5f);
    centerText(sfxStop, windowWidth / 2.0f + 50.0f, windowHeight / 2.0f + 150.0f);

    Text soundReset(font);
    soundReset.setString(L"ZRESETUJ");
    soundReset.setCharacterSize(50);
    soundReset.setFillColor(Color(0xFFFFFFFF));
    soundReset.setOutlineColor(Color(0x000000FF));
    soundReset.setOutlineThickness(1.5f);
    centerText(soundReset, windowWidth / 2.0f, windowHeight / 2.0f + 250.0f);

    Text volumeSettingsExit(font);
    volumeSettingsExit.setString(L"WYJåCIE");
    volumeSettingsExit.setCharacterSize(50);
    volumeSettingsExit.setFillColor(Color::White);
    volumeSettingsExit.setOutlineColor(Color::White);
    volumeSettingsExit.setOutlineThickness(1.5f);
    centerText(volumeSettingsExit, windowWidth / 2.0f, windowHeight / 2.0f + 350.0f);


    bool inMainMenu = true;



    // --- P TLA MENU G£”WNEGO ---
    while (window.isOpen() && inMainMenu) {
        sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window); // Update pozycji myszki na poczπtku klatki

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) { window.close(); return 0; }
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->scancode == sf::Keyboard::Scancode::F11) {
                    isFullscreen = !isFullscreen;
                    window.close();
                    if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                    else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                    window.setFramerateLimit(140);
                }
            }
            // ZMIANA: Obs≥uga klikniÍcia jako ZDARZENIE (raz na klik), a nie stan
            else if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>()) {
                if (mouseBtn->button == Mouse::Button::Left) {
                    if (menuStart.getGlobalBounds().contains(mousePos)) {                                                              // warunek w ktÛrym po klikniÍciu start dzieje siÍ reszta
                        clickSound.play();                                                                                             //
                        inMainMenu = false; // Przejúcie do nastÍpnego ekranu                                                          
                    }

                    // --- Menu ustawieÒ ---

                    if (menuSettings.getGlobalBounds().contains(mousePos))
                    {
                        clickSound.play();

                        bool inSettings = true;

                        // --- W ustawieniach ---
                        while (window.isOpen() && inSettings)
                        {
                            sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);   // aktualizacja pozycji myszki

                            while (const optional event = window.pollEvent())
                            {
                                if (event->is<Event::Closed>())
                                {
                                    clickSound.play();
                                    inSettings = false;
                                }
                                else if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                {
                                    if (mouseBtn->button == Mouse::Button::Left)
                                    {
                                        // klikniÍcie PRZYPISYWANIE KLAWISZY
                                        if (keyBindings.getGlobalBounds().contains(mousePos))
                                        {
                                            clickSound.play();
                                            bool inKeyBindings = true;

                                            while (window.isOpen() && inKeyBindings)
                                            {
                                                sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

                                                while (const optional event = window.pollEvent())
                                                {
                                                    if (event->is<Event::Closed>())
                                                    {
                                                        clickSound.play();
                                                        inKeyBindings = false;
                                                    }
                                                    else if (const auto* key = event->getIf<Event::KeyPressed>())
                                                    {
                                                        if (key->scancode == Keyboard::Scancode::Escape)
                                                        {
                                                            inKeyBindings = false;
                                                        }
                                                        if (key->scancode == Keyboard::Scancode::F11) {
                                                            isFullscreen = !isFullscreen;
                                                            window.close();
                                                            if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                                                            else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                                                            window.setFramerateLimit(140);
                                                        }
                                                    }
                                                    // klikniÍcie SKOK 1
                                                    if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                                    {
                                                        if (mouseBtn->button == Mouse::Button::Left)
                                                        {
                                                            if (Jump1.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();

                                                                Texture screenshotTexture;
                                                                if (screenshotTexture.resize(window.getSize())) screenshotTexture.update(window);
                                                                Sprite tlo(screenshotTexture);

                                                                tlo.setColor(Color(50, 50, 50));

                                                                bool inControls = true;

                                                                while (window.isOpen() && inControls)
                                                                {
                                                                    sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

                                                                    while (const optional event = window.pollEvent())
                                                                    {
                                                                        if (event->is<Event::Closed>())
                                                                        {
                                                                            clickSound.play();
                                                                            inControls = false;
                                                                        }
                                                                        else if (const auto* key = event->getIf<Event::KeyPressed>())
                                                                        {
                                                                            if (key->scancode == Keyboard::Scancode::Escape)
                                                                            {
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::Delete)
                                                                            {
                                                                                keyJump1 = Keyboard::Scancode::Unknown;
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::F11) {
                                                                                isFullscreen = !isFullscreen;
                                                                                window.close();
                                                                                if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                                                                                else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                                                                                window.setFramerateLimit(140);
                                                                            }
                                                                            else if (
                                                                                key->scancode == Keyboard::Scancode::PrintScreen ||
                                                                                key->scancode == Keyboard::Scancode::ScrollLock ||
                                                                                key->scancode == Keyboard::Scancode::Pause ||
                                                                                key->scancode == Keyboard::Scancode::Insert ||
                                                                                key->scancode == Keyboard::Scancode::Home ||
                                                                                key->scancode == Keyboard::Scancode::PageUp ||
                                                                                key->scancode == Keyboard::Scancode::End ||
                                                                                key->scancode == Keyboard::Scancode::PageDown ||
                                                                                key->scancode == Keyboard::Scancode::F11
                                                                                )
                                                                            {
                                                                            }
                                                                            else
                                                                            {
                                                                                keyJump1 = key->scancode;
                                                                                inControls = false;
                                                                            }
                                                                        }
                                                                    }
                                                                    window.clear(Color(0x808080FF));

                                                                    window.draw(tlo);
                                                                    window.draw(ControlsTitle);

                                                                    window.display();
                                                                }
                                                                String newKeyName;

                                                                if (keyJump1 == Keyboard::Scancode::Unknown)
                                                                {
                                                                    newKeyName = L"Unknown";
                                                                }
                                                                else
                                                                {
                                                                    newKeyName = Keyboard::getDescription(keyJump1);
                                                                }

                                                                Jump1.setString(L"SKOK (" + newKeyName + ")");
                                                                centerText(Jump1, windowWidth / 2.0f - 500.0f, windowHeight / 2.0f - 250.0f);
                                                            }
                                                        }
                                                    }
                                                    // klikniÍcie SKOK 2
                                                    if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                                    {
                                                        if (mouseBtn->button == Mouse::Button::Left)
                                                        {
                                                            if (Jump2.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();

                                                                Texture screenshotTexture;
                                                                if (screenshotTexture.resize(window.getSize())) screenshotTexture.update(window);
                                                                Sprite tlo(screenshotTexture);

                                                                tlo.setColor(Color(50, 50, 50));

                                                                bool inControls = true;

                                                                while (window.isOpen() && inControls)
                                                                {
                                                                    sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

                                                                    while (const optional event = window.pollEvent())
                                                                    {
                                                                        if (event->is<Event::Closed>())
                                                                        {
                                                                            clickSound.play();
                                                                            inControls = false;
                                                                        }
                                                                        else if (const auto* key = event->getIf<Event::KeyPressed>())
                                                                        {
                                                                            if (key->scancode == Keyboard::Scancode::Escape)
                                                                            {
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::Delete)
                                                                            {
                                                                                keyJump2 = Keyboard::Scancode::Unknown;
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::F11) {
                                                                                isFullscreen = !isFullscreen;
                                                                                window.close();
                                                                                if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                                                                                else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                                                                                window.setFramerateLimit(140);
                                                                            }
                                                                            else if (
                                                                                key->scancode == Keyboard::Scancode::PrintScreen ||
                                                                                key->scancode == Keyboard::Scancode::ScrollLock ||
                                                                                key->scancode == Keyboard::Scancode::Pause ||
                                                                                key->scancode == Keyboard::Scancode::Insert ||
                                                                                key->scancode == Keyboard::Scancode::Home ||
                                                                                key->scancode == Keyboard::Scancode::PageUp ||
                                                                                key->scancode == Keyboard::Scancode::End ||
                                                                                key->scancode == Keyboard::Scancode::PageDown ||
                                                                                key->scancode == Keyboard::Scancode::F11
                                                                                )
                                                                            {
                                                                            }
                                                                            else
                                                                            {
                                                                                keyJump2 = key->scancode;
                                                                                inControls = false;
                                                                            }
                                                                        }
                                                                    }
                                                                    window.clear(Color(0x808080FF));

                                                                    window.draw(tlo);
                                                                    window.draw(ControlsTitle);

                                                                    window.display();
                                                                }
                                                                String newKeyName;

                                                                if (keyJump2 == Keyboard::Scancode::Unknown)
                                                                {
                                                                    newKeyName = L"Unknown";
                                                                }
                                                                else
                                                                {
                                                                    newKeyName = Keyboard::getDescription(keyJump2);
                                                                }

                                                                Jump2.setString(L"SKOK (" + newKeyName + ")");
                                                                centerText(Jump2, windowWidth / 2.0f, windowHeight / 2.0f - 250.0f);
                                                            }
                                                        }
                                                    }
                                                    // klikniÍcie SKOK 3
                                                    if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                                    {
                                                        if (mouseBtn->button == Mouse::Button::Left)
                                                        {
                                                            if (Jump3.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();

                                                                Texture screenshotTexture;
                                                                if (screenshotTexture.resize(window.getSize())) screenshotTexture.update(window);
                                                                Sprite tlo(screenshotTexture);

                                                                tlo.setColor(Color(50, 50, 50));

                                                                bool inControls = true;

                                                                while (window.isOpen() && inControls)
                                                                {
                                                                    sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

                                                                    while (const optional event = window.pollEvent())
                                                                    {
                                                                        if (event->is<Event::Closed>())
                                                                        {
                                                                            clickSound.play();
                                                                            inControls = false;
                                                                        }
                                                                        else if (const auto* key = event->getIf<Event::KeyPressed>())
                                                                        {
                                                                            if (key->scancode == Keyboard::Scancode::Escape)
                                                                            {
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::Delete)
                                                                            {
                                                                                keyJump3 = Keyboard::Scancode::Unknown;
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::F11) {
                                                                                isFullscreen = !isFullscreen;
                                                                                window.close();
                                                                                if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                                                                                else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                                                                                window.setFramerateLimit(140);
                                                                            }
                                                                            else if (
                                                                                key->scancode == Keyboard::Scancode::PrintScreen ||
                                                                                key->scancode == Keyboard::Scancode::ScrollLock ||
                                                                                key->scancode == Keyboard::Scancode::Pause ||
                                                                                key->scancode == Keyboard::Scancode::Insert ||
                                                                                key->scancode == Keyboard::Scancode::Home ||
                                                                                key->scancode == Keyboard::Scancode::PageUp ||
                                                                                key->scancode == Keyboard::Scancode::End ||
                                                                                key->scancode == Keyboard::Scancode::PageDown ||
                                                                                key->scancode == Keyboard::Scancode::F11
                                                                                )
                                                                            {
                                                                            }
                                                                            else
                                                                            {
                                                                                keyJump3 = key->scancode;
                                                                                inControls = false;
                                                                            }
                                                                        }
                                                                    }
                                                                    window.clear(Color(0x808080FF));

                                                                    window.draw(tlo);
                                                                    window.draw(ControlsTitle);

                                                                    window.display();
                                                                }
                                                                String newKeyName;

                                                                if (keyJump3 == Keyboard::Scancode::Unknown)
                                                                {
                                                                    newKeyName = L"Unknown";
                                                                }
                                                                else
                                                                {
                                                                    newKeyName = Keyboard::getDescription(keyJump3);
                                                                }

                                                                Jump3.setString(L"SKOK (" + newKeyName + ")");
                                                                centerText(Jump3, windowWidth / 2.0f + 500.0f, windowHeight / 2.0f - 250.0f);
                                                            }
                                                        }
                                                    }
                                                    // klikniÍcie PRAWO 1
                                                    if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                                    {
                                                        if (mouseBtn->button == Mouse::Button::Left)
                                                        {
                                                            if (Right1.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();

                                                                Texture screenshotTexture;
                                                                if (screenshotTexture.resize(window.getSize())) screenshotTexture.update(window);
                                                                Sprite tlo(screenshotTexture);

                                                                tlo.setColor(Color(50, 50, 50));

                                                                bool inControls = true;

                                                                while (window.isOpen() && inControls)
                                                                {
                                                                    sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

                                                                    while (const optional event = window.pollEvent())
                                                                    {
                                                                        if (event->is<Event::Closed>())
                                                                        {
                                                                            clickSound.play();
                                                                            inControls = false;
                                                                        }
                                                                        else if (const auto* key = event->getIf<Event::KeyPressed>())
                                                                        {
                                                                            if (key->scancode == Keyboard::Scancode::Escape)
                                                                            {
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::Delete)
                                                                            {
                                                                                keyRight1 = Keyboard::Scancode::Unknown;
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::F11) {
                                                                                isFullscreen = !isFullscreen;
                                                                                window.close();
                                                                                if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                                                                                else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                                                                                window.setFramerateLimit(140);
                                                                            }
                                                                            else if (
                                                                                key->scancode == Keyboard::Scancode::PrintScreen ||
                                                                                key->scancode == Keyboard::Scancode::ScrollLock ||
                                                                                key->scancode == Keyboard::Scancode::Pause ||
                                                                                key->scancode == Keyboard::Scancode::Insert ||
                                                                                key->scancode == Keyboard::Scancode::Home ||
                                                                                key->scancode == Keyboard::Scancode::PageUp ||
                                                                                key->scancode == Keyboard::Scancode::End ||
                                                                                key->scancode == Keyboard::Scancode::PageDown ||
                                                                                key->scancode == Keyboard::Scancode::F11
                                                                                )
                                                                            {
                                                                            }
                                                                            else
                                                                            {
                                                                                keyRight1 = key->scancode;
                                                                                inControls = false;
                                                                            }
                                                                        }
                                                                    }
                                                                    window.clear(Color(0x808080FF));

                                                                    window.draw(tlo);
                                                                    window.draw(ControlsTitle);

                                                                    window.display();
                                                                }
                                                                String newKeyName;

                                                                if (keyRight1 == Keyboard::Scancode::Unknown)
                                                                {
                                                                    newKeyName = L"Unknown";
                                                                }
                                                                else
                                                                {
                                                                    newKeyName = Keyboard::getDescription(keyRight1);
                                                                }

                                                                Right1.setString(L"PRAWO (" + newKeyName + ")");
                                                                centerText(Right1, windowWidth / 2.0f - 500.0f, windowHeight / 2.0f + 50.0f);
                                                            }
                                                        }
                                                    }
                                                    // klikniÍcie PRAWO 2
                                                    if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                                    {
                                                        if (mouseBtn->button == Mouse::Button::Left)
                                                        {
                                                            if (Right2.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();

                                                                Texture screenshotTexture;
                                                                if (screenshotTexture.resize(window.getSize())) screenshotTexture.update(window);
                                                                Sprite tlo(screenshotTexture);

                                                                tlo.setColor(Color(50, 50, 50));

                                                                bool inControls = true;

                                                                while (window.isOpen() && inControls)
                                                                {
                                                                    sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

                                                                    while (const optional event = window.pollEvent())
                                                                    {
                                                                        if (event->is<Event::Closed>())
                                                                        {
                                                                            clickSound.play();
                                                                            inControls = false;
                                                                        }
                                                                        else if (const auto* key = event->getIf<Event::KeyPressed>())
                                                                        {
                                                                            if (key->scancode == Keyboard::Scancode::Escape)
                                                                            {
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::Delete)
                                                                            {
                                                                                keyRight2 = Keyboard::Scancode::Unknown;
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::F11) {
                                                                                isFullscreen = !isFullscreen;
                                                                                window.close();
                                                                                if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                                                                                else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                                                                                window.setFramerateLimit(140);
                                                                            }
                                                                            else if (
                                                                                key->scancode == Keyboard::Scancode::PrintScreen ||
                                                                                key->scancode == Keyboard::Scancode::ScrollLock ||
                                                                                key->scancode == Keyboard::Scancode::Pause ||
                                                                                key->scancode == Keyboard::Scancode::Insert ||
                                                                                key->scancode == Keyboard::Scancode::Home ||
                                                                                key->scancode == Keyboard::Scancode::PageUp ||
                                                                                key->scancode == Keyboard::Scancode::End ||
                                                                                key->scancode == Keyboard::Scancode::PageDown ||
                                                                                key->scancode == Keyboard::Scancode::F11
                                                                                )
                                                                            {
                                                                            }
                                                                            else
                                                                            {
                                                                                keyRight2 = key->scancode;
                                                                                inControls = false;
                                                                            }
                                                                        }
                                                                    }
                                                                    window.clear(Color(0x808080FF));

                                                                    window.draw(tlo);
                                                                    window.draw(ControlsTitle);

                                                                    window.display();
                                                                }
                                                                String newKeyName;

                                                                if (keyRight2 == Keyboard::Scancode::Unknown)
                                                                {
                                                                    newKeyName = L"Unknown";
                                                                }
                                                                else
                                                                {
                                                                    newKeyName = Keyboard::getDescription(keyRight2);
                                                                }

                                                                Right2.setString(L"PRAWO (" + newKeyName + ")");
                                                                centerText(Right2, windowWidth / 2.0f, windowHeight / 2.0f + 50.0f);
                                                            }
                                                        }
                                                    }
                                                    // klikniÍcie PRAWO 3
                                                    if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                                    {
                                                        if (mouseBtn->button == Mouse::Button::Left)
                                                        {
                                                            if (Right3.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();

                                                                Texture screenshotTexture;
                                                                if (screenshotTexture.resize(window.getSize())) screenshotTexture.update(window);
                                                                Sprite tlo(screenshotTexture);

                                                                tlo.setColor(Color(50, 50, 50));

                                                                bool inControls = true;

                                                                while (window.isOpen() && inControls)
                                                                {
                                                                    sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

                                                                    while (const optional event = window.pollEvent())
                                                                    {
                                                                        if (event->is<Event::Closed>())
                                                                        {
                                                                            clickSound.play();
                                                                            inControls = false;
                                                                        }
                                                                        else if (const auto* key = event->getIf<Event::KeyPressed>())
                                                                        {
                                                                            if (key->scancode == Keyboard::Scancode::Escape)
                                                                            {
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::Delete)
                                                                            {
                                                                                keyRight3 = Keyboard::Scancode::Unknown;
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::F11) {
                                                                                isFullscreen = !isFullscreen;
                                                                                window.close();
                                                                                if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                                                                                else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                                                                                window.setFramerateLimit(140);
                                                                            }
                                                                            else if (
                                                                                key->scancode == Keyboard::Scancode::PrintScreen ||
                                                                                key->scancode == Keyboard::Scancode::ScrollLock ||
                                                                                key->scancode == Keyboard::Scancode::Pause ||
                                                                                key->scancode == Keyboard::Scancode::Insert ||
                                                                                key->scancode == Keyboard::Scancode::Home ||
                                                                                key->scancode == Keyboard::Scancode::PageUp ||
                                                                                key->scancode == Keyboard::Scancode::End ||
                                                                                key->scancode == Keyboard::Scancode::PageDown ||
                                                                                key->scancode == Keyboard::Scancode::F11
                                                                                )
                                                                            {
                                                                            }
                                                                            else
                                                                            {
                                                                                keyRight3 = key->scancode;
                                                                                inControls = false;
                                                                            }
                                                                        }
                                                                    }
                                                                    window.clear(Color(0x808080FF));

                                                                    window.draw(tlo);
                                                                    window.draw(ControlsTitle);

                                                                    window.display();
                                                                }
                                                                String newKeyName;

                                                                if (keyRight3 == Keyboard::Scancode::Unknown)
                                                                {
                                                                    newKeyName = L"Unknown";
                                                                }
                                                                else
                                                                {
                                                                    newKeyName = Keyboard::getDescription(keyRight3);
                                                                }

                                                                Right3.setString(L"PRAWO (" + newKeyName + ")");
                                                                centerText(Right3, windowWidth / 2.0f + 500.0f, windowHeight / 2.0f + 50.0f);
                                                            }
                                                        }
                                                    }
                                                    // klikniÍcie LEWO 1
                                                    if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                                    {
                                                        if (mouseBtn->button == Mouse::Button::Left)
                                                        {
                                                            if (Left1.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();

                                                                Texture screenshotTexture;
                                                                if (screenshotTexture.resize(window.getSize())) screenshotTexture.update(window);
                                                                Sprite tlo(screenshotTexture);

                                                                tlo.setColor(Color(50, 50, 50));

                                                                bool inControls = true;

                                                                while (window.isOpen() && inControls)
                                                                {
                                                                    sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

                                                                    while (const optional event = window.pollEvent())
                                                                    {
                                                                        if (event->is<Event::Closed>())
                                                                        {
                                                                            clickSound.play();
                                                                            inControls = false;
                                                                        }
                                                                        else if (const auto* key = event->getIf<Event::KeyPressed>())
                                                                        {
                                                                            if (key->scancode == Keyboard::Scancode::Escape)
                                                                            {
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::Delete)
                                                                            {
                                                                                keyLeft1 = Keyboard::Scancode::Unknown;
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::F11) {
                                                                                isFullscreen = !isFullscreen;
                                                                                window.close();
                                                                                if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                                                                                else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                                                                                window.setFramerateLimit(140);
                                                                            }
                                                                            else if (
                                                                                key->scancode == Keyboard::Scancode::PrintScreen ||
                                                                                key->scancode == Keyboard::Scancode::ScrollLock ||
                                                                                key->scancode == Keyboard::Scancode::Pause ||
                                                                                key->scancode == Keyboard::Scancode::Insert ||
                                                                                key->scancode == Keyboard::Scancode::Home ||
                                                                                key->scancode == Keyboard::Scancode::PageUp ||
                                                                                key->scancode == Keyboard::Scancode::End ||
                                                                                key->scancode == Keyboard::Scancode::PageDown ||
                                                                                key->scancode == Keyboard::Scancode::F11
                                                                                )
                                                                            {
                                                                            }
                                                                            else
                                                                            {
                                                                                keyLeft1 = key->scancode;
                                                                                inControls = false;
                                                                            }
                                                                        }
                                                                    }
                                                                    window.clear(Color(0x808080FF));

                                                                    window.draw(tlo);
                                                                    window.draw(ControlsTitle);

                                                                    window.display();
                                                                }
                                                                String newKeyName;

                                                                if (keyLeft1 == Keyboard::Scancode::Unknown)
                                                                {
                                                                    newKeyName = L"Unknown";
                                                                }
                                                                else
                                                                {
                                                                    newKeyName = Keyboard::getDescription(keyLeft1);
                                                                }

                                                                Left1.setString(L"LEWO (" + newKeyName + ")");
                                                                centerText(Left1, windowWidth / 2.0f - 500.0f, windowHeight / 2.0f - 150.0f);
                                                            }
                                                        }
                                                    }
                                                    // klikniÍcie LEWO 2
                                                    if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                                    {
                                                        if (mouseBtn->button == Mouse::Button::Left)
                                                        {
                                                            if (Left2.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();

                                                                Texture screenshotTexture;
                                                                if (screenshotTexture.resize(window.getSize())) screenshotTexture.update(window);
                                                                Sprite tlo(screenshotTexture);

                                                                tlo.setColor(Color(50, 50, 50));

                                                                bool inControls = true;

                                                                while (window.isOpen() && inControls)
                                                                {
                                                                    sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

                                                                    while (const optional event = window.pollEvent())
                                                                    {
                                                                        if (event->is<Event::Closed>())
                                                                        {
                                                                            clickSound.play();
                                                                            inControls = false;
                                                                        }
                                                                        else if (const auto* key = event->getIf<Event::KeyPressed>())
                                                                        {
                                                                            if (key->scancode == Keyboard::Scancode::Escape)
                                                                            {
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::Delete)
                                                                            {
                                                                                keyLeft2 = Keyboard::Scancode::Unknown;
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::F11) {
                                                                                isFullscreen = !isFullscreen;
                                                                                window.close();
                                                                                if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                                                                                else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                                                                                window.setFramerateLimit(140);
                                                                            }
                                                                            else if (
                                                                                key->scancode == Keyboard::Scancode::PrintScreen ||
                                                                                key->scancode == Keyboard::Scancode::ScrollLock ||
                                                                                key->scancode == Keyboard::Scancode::Pause ||
                                                                                key->scancode == Keyboard::Scancode::Insert ||
                                                                                key->scancode == Keyboard::Scancode::Home ||
                                                                                key->scancode == Keyboard::Scancode::PageUp ||
                                                                                key->scancode == Keyboard::Scancode::End ||
                                                                                key->scancode == Keyboard::Scancode::PageDown ||
                                                                                key->scancode == Keyboard::Scancode::F11
                                                                                )
                                                                            {
                                                                            }
                                                                            else
                                                                            {
                                                                                keyLeft2 = key->scancode;
                                                                                inControls = false;
                                                                            }
                                                                        }
                                                                    }
                                                                    window.clear(Color(0x808080FF));

                                                                    window.draw(tlo);
                                                                    window.draw(ControlsTitle);

                                                                    window.display();
                                                                }
                                                                String newKeyName;

                                                                if (keyLeft2 == Keyboard::Scancode::Unknown)
                                                                {
                                                                    newKeyName = L"Unknown";
                                                                }
                                                                else
                                                                {
                                                                    newKeyName = Keyboard::getDescription(keyLeft2);
                                                                }

                                                                Left2.setString(L"LEWO (" + newKeyName + ")");
                                                                centerText(Left2, windowWidth / 2.0f, windowHeight / 2.0f - 150.0f);
                                                            }
                                                        }
                                                    }
                                                    // klikniÍcie LEWO 3
                                                    if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                                    {
                                                        if (mouseBtn->button == Mouse::Button::Left)
                                                        {
                                                            if (Left3.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();

                                                                Texture screenshotTexture;
                                                                if (screenshotTexture.resize(window.getSize())) screenshotTexture.update(window);
                                                                Sprite tlo(screenshotTexture);

                                                                tlo.setColor(Color(50, 50, 50));

                                                                bool inControls = true;

                                                                while (window.isOpen() && inControls)
                                                                {
                                                                    sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

                                                                    while (const optional event = window.pollEvent())
                                                                    {
                                                                        if (event->is<Event::Closed>())
                                                                        {
                                                                            clickSound.play();
                                                                            inControls = false;
                                                                        }
                                                                        else if (const auto* key = event->getIf<Event::KeyPressed>())
                                                                        {
                                                                            if (key->scancode == Keyboard::Scancode::Escape)
                                                                            {
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::Delete)
                                                                            {
                                                                                keyLeft3 = Keyboard::Scancode::Unknown;
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::F11) {
                                                                                isFullscreen = !isFullscreen;
                                                                                window.close();
                                                                                if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                                                                                else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                                                                                window.setFramerateLimit(140);
                                                                            }
                                                                            else if (
                                                                                key->scancode == Keyboard::Scancode::PrintScreen ||
                                                                                key->scancode == Keyboard::Scancode::ScrollLock ||
                                                                                key->scancode == Keyboard::Scancode::Pause ||
                                                                                key->scancode == Keyboard::Scancode::Insert ||
                                                                                key->scancode == Keyboard::Scancode::Home ||
                                                                                key->scancode == Keyboard::Scancode::PageUp ||
                                                                                key->scancode == Keyboard::Scancode::End ||
                                                                                key->scancode == Keyboard::Scancode::PageDown ||
                                                                                key->scancode == Keyboard::Scancode::F11
                                                                                )
                                                                            {
                                                                            }
                                                                            else
                                                                            {
                                                                                keyLeft3 = key->scancode;
                                                                                inControls = false;
                                                                            }
                                                                        }
                                                                    }
                                                                    window.clear(Color(0x808080FF));

                                                                    window.draw(tlo);
                                                                    window.draw(ControlsTitle);

                                                                    window.display();
                                                                }
                                                                String newKeyName;

                                                                if (keyLeft3 == Keyboard::Scancode::Unknown)
                                                                {
                                                                    newKeyName = L"Unknown";
                                                                }
                                                                else
                                                                {
                                                                    newKeyName = Keyboard::getDescription(keyLeft3);
                                                                }

                                                                Left3.setString(L"LEWO (" + newKeyName + ")");
                                                                centerText(Left3, windowWidth / 2.0f + 500.0f, windowHeight / 2.0f - 150.0f);
                                                            }
                                                        }
                                                    }
                                                    // klikniÍcie BIEG 1
                                                    if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                                    {
                                                        if (mouseBtn->button == Mouse::Button::Left)
                                                        {
                                                            if (Run1.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();

                                                                Texture screenshotTexture;
                                                                if (screenshotTexture.resize(window.getSize())) screenshotTexture.update(window);
                                                                Sprite tlo(screenshotTexture);

                                                                tlo.setColor(Color(50, 50, 50));

                                                                bool inControls = true;

                                                                while (window.isOpen() && inControls)
                                                                {
                                                                    sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

                                                                    while (const optional event = window.pollEvent())
                                                                    {
                                                                        if (event->is<Event::Closed>())
                                                                        {
                                                                            clickSound.play();
                                                                            inControls = false;
                                                                        }
                                                                        else if (const auto* key = event->getIf<Event::KeyPressed>())
                                                                        {
                                                                            if (key->scancode == Keyboard::Scancode::Escape)
                                                                            {
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::Delete)
                                                                            {
                                                                                keyRun1 = Keyboard::Scancode::Unknown;
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::F11) {
                                                                                isFullscreen = !isFullscreen;
                                                                                window.close();
                                                                                if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                                                                                else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                                                                                window.setFramerateLimit(140);
                                                                            }
                                                                            else if (
                                                                                key->scancode == Keyboard::Scancode::PrintScreen ||
                                                                                key->scancode == Keyboard::Scancode::ScrollLock ||
                                                                                key->scancode == Keyboard::Scancode::Pause ||
                                                                                key->scancode == Keyboard::Scancode::Insert ||
                                                                                key->scancode == Keyboard::Scancode::Home ||
                                                                                key->scancode == Keyboard::Scancode::PageUp ||
                                                                                key->scancode == Keyboard::Scancode::End ||
                                                                                key->scancode == Keyboard::Scancode::PageDown ||
                                                                                key->scancode == Keyboard::Scancode::F11
                                                                                )
                                                                            {
                                                                            }
                                                                            else
                                                                            {
                                                                                keyRun1 = key->scancode;
                                                                                inControls = false;
                                                                            }
                                                                        }
                                                                    }
                                                                    window.clear(Color(0x808080FF));

                                                                    window.draw(tlo);
                                                                    window.draw(ControlsTitle);

                                                                    window.display();
                                                                }
                                                                String newKeyName;

                                                                if (keyRun1 == Keyboard::Scancode::Unknown)
                                                                {
                                                                    newKeyName = L"Unknown";
                                                                }
                                                                else
                                                                {
                                                                    newKeyName = Keyboard::getDescription(keyRun1);
                                                                }

                                                                Run1.setString(L"BIEG (" + newKeyName + ")");
                                                                centerText(Run1, windowWidth / 2.0f - 500.0f, windowHeight / 2.0f - 50.0f);
                                                            }
                                                        }
                                                    }
                                                    // klikniÍcie BIEG 2
                                                    if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                                    {
                                                        if (mouseBtn->button == Mouse::Button::Left)
                                                        {
                                                            if (Run2.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();

                                                                Texture screenshotTexture;
                                                                if (screenshotTexture.resize(window.getSize())) screenshotTexture.update(window);
                                                                Sprite tlo(screenshotTexture);

                                                                tlo.setColor(Color(50, 50, 50));

                                                                bool inControls = true;

                                                                while (window.isOpen() && inControls)
                                                                {
                                                                    sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

                                                                    while (const optional event = window.pollEvent())
                                                                    {
                                                                        if (event->is<Event::Closed>())
                                                                        {
                                                                            clickSound.play();
                                                                            inControls = false;
                                                                        }
                                                                        else if (const auto* key = event->getIf<Event::KeyPressed>())
                                                                        {
                                                                            if (key->scancode == Keyboard::Scancode::Escape)
                                                                            {
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::Delete)
                                                                            {
                                                                                keyRun2 = Keyboard::Scancode::Unknown;
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::F11) {
                                                                                isFullscreen = !isFullscreen;
                                                                                window.close();
                                                                                if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                                                                                else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                                                                                window.setFramerateLimit(140);
                                                                            }
                                                                            else if (
                                                                                key->scancode == Keyboard::Scancode::PrintScreen ||
                                                                                key->scancode == Keyboard::Scancode::ScrollLock ||
                                                                                key->scancode == Keyboard::Scancode::Pause ||
                                                                                key->scancode == Keyboard::Scancode::Insert ||
                                                                                key->scancode == Keyboard::Scancode::Home ||
                                                                                key->scancode == Keyboard::Scancode::PageUp ||
                                                                                key->scancode == Keyboard::Scancode::End ||
                                                                                key->scancode == Keyboard::Scancode::PageDown ||
                                                                                key->scancode == Keyboard::Scancode::F11
                                                                                )
                                                                            {
                                                                            }
                                                                            else
                                                                            {
                                                                                keyRun2 = key->scancode;
                                                                                inControls = false;
                                                                            }
                                                                        }
                                                                    }
                                                                    window.clear(Color(0x808080FF));

                                                                    window.draw(tlo);
                                                                    window.draw(ControlsTitle);

                                                                    window.display();
                                                                }
                                                                String newKeyName;

                                                                if (keyRun2 == Keyboard::Scancode::Unknown)
                                                                {
                                                                    newKeyName = L"Unknown";
                                                                }
                                                                else
                                                                {
                                                                    newKeyName = Keyboard::getDescription(keyRun2);
                                                                }

                                                                Run2.setString(L"BIEG (" + newKeyName + ")");
                                                                centerText(Run2, windowWidth / 2.0f, windowHeight / 2.0f - 50.0f);
                                                            }
                                                        }
                                                    }
                                                    // klikniÍcie BIEG 3
                                                    if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                                    {
                                                        if (mouseBtn->button == Mouse::Button::Left)
                                                        {
                                                            if (Run3.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();

                                                                Texture screenshotTexture;
                                                                if (screenshotTexture.resize(window.getSize())) screenshotTexture.update(window);
                                                                Sprite tlo(screenshotTexture);

                                                                tlo.setColor(Color(50, 50, 50));

                                                                bool inControls = true;

                                                                while (window.isOpen() && inControls)
                                                                {
                                                                    sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

                                                                    while (const optional event = window.pollEvent())
                                                                    {
                                                                        if (event->is<Event::Closed>())
                                                                        {
                                                                            clickSound.play();
                                                                            inControls = false;
                                                                        }
                                                                        else if (const auto* key = event->getIf<Event::KeyPressed>())
                                                                        {
                                                                            if (key->scancode == Keyboard::Scancode::Escape)
                                                                            {
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::Delete)
                                                                            {
                                                                                keyRun3 = Keyboard::Scancode::Unknown;
                                                                                inControls = false;
                                                                            }
                                                                            else if (key->scancode == Keyboard::Scancode::F11) {
                                                                                isFullscreen = !isFullscreen;
                                                                                window.close();
                                                                                if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                                                                                else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                                                                                window.setFramerateLimit(140);
                                                                            }
                                                                            else if (
                                                                                key->scancode == Keyboard::Scancode::PrintScreen ||
                                                                                key->scancode == Keyboard::Scancode::ScrollLock ||
                                                                                key->scancode == Keyboard::Scancode::Pause ||
                                                                                key->scancode == Keyboard::Scancode::Insert ||
                                                                                key->scancode == Keyboard::Scancode::Home ||
                                                                                key->scancode == Keyboard::Scancode::PageUp ||
                                                                                key->scancode == Keyboard::Scancode::End ||
                                                                                key->scancode == Keyboard::Scancode::PageDown ||
                                                                                key->scancode == Keyboard::Scancode::F11
                                                                                )
                                                                            {
                                                                            }
                                                                            else
                                                                            {
                                                                                keyRun3 = key->scancode;
                                                                                inControls = false;
                                                                            }
                                                                        }
                                                                    }
                                                                    window.clear(Color(0x808080FF));

                                                                    window.draw(tlo);
                                                                    window.draw(ControlsTitle);

                                                                    window.display();
                                                                }
                                                                String newKeyName;

                                                                if (keyRun3 == Keyboard::Scancode::Unknown)
                                                                {
                                                                    newKeyName = L"Unknown";
                                                                }
                                                                else
                                                                {
                                                                    newKeyName = Keyboard::getDescription(keyRun3);
                                                                }

                                                                Run3.setString(L"BIEG (" + newKeyName + ")");
                                                                centerText(Run3, windowWidth / 2.0f + 500.0f, windowHeight / 2.0f - 50.0f);
                                                            }
                                                        }
                                                    }
                                                    if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                                    {
                                                        if (mouseBtn->button == Mouse::Button::Left)
                                                        {
                                                            if (keyBindingsReset.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();

                                                                // Zmiana (mechaniczna) klawiszy 
                                                                keyJump1 = Keyboard::Scancode::Up;
                                                                keyJump2 = Keyboard::Scancode::W;
                                                                keyJump3 = Keyboard::Scancode::Space;

                                                                keyLeft1 = Keyboard::Scancode::Left;
                                                                keyLeft2 = Keyboard::Scancode::A;
                                                                keyLeft3 = Keyboard::Scancode::Unknown;

                                                                keyRun1 = Keyboard::Scancode::Down;
                                                                keyRun2 = Keyboard::Scancode::S;
                                                                keyRun3 = Keyboard::Scancode::LShift;

                                                                keyRight1 = Keyboard::Scancode::Right;
                                                                keyRight2 = Keyboard::Scancode::D;
                                                                keyRight3 = Keyboard::Scancode::Unknown;

                                                                // Zmiana (graficzna) klawiszy 
                                                                Jump1.setString(L"SKOK (" + Keyboard::getDescription(keyJump1) + L")");
                                                                centerText(Jump1, windowWidth / 2.0f - 500.0f, windowHeight / 2.0f - 250.0f);
                                                                Jump2.setString(L"SKOK (" + Keyboard::getDescription(keyJump2) + L")");
                                                                centerText(Jump2, windowWidth / 2.0f, windowHeight / 2.0f - 250.0f);
                                                                Jump3.setString(L"SKOK (" + Keyboard::getDescription(keyJump3) + L")");
                                                                centerText(Jump3, windowWidth / 2.0f + 500.0f, windowHeight / 2.0f - 250.0f);

                                                                Left1.setString(L"LEWO (" + Keyboard::getDescription(keyLeft1) + L")");
                                                                centerText(Left1, windowWidth / 2.0f - 500.0f, windowHeight / 2.0f - 150.0f);
                                                                Left2.setString(L"LEWO (" + Keyboard::getDescription(keyLeft2) + L")");
                                                                centerText(Left2, windowWidth / 2.0f, windowHeight / 2.0f - 150.0f);
                                                                Left3.setString(L"LEWO (" + Keyboard::getDescription(keyLeft3) + L")");
                                                                centerText(Left3, windowWidth / 2.0f + 500.0f, windowHeight / 2.0f - 150.0f);

                                                                Run1.setString(L"BIEG (" + Keyboard::getDescription(keyRun1) + L")");
                                                                centerText(Run1, windowWidth / 2.0f - 500.0f, windowHeight / 2.0f - 50.0f);
                                                                Run2.setString(L"BIEG (" + Keyboard::getDescription(keyRun2) + L")");
                                                                centerText(Run2, windowWidth / 2.0f, windowHeight / 2.0f - 50.0f);
                                                                Run3.setString(L"BIEG (" + Keyboard::getDescription(keyRun3) + L")");
                                                                centerText(Run3, windowWidth / 2.0f + 500.0f, windowHeight / 2.0f - 50.0f);

                                                                Right1.setString(L"PRAWO (" + Keyboard::getDescription(keyRight1) + L")");
                                                                centerText(Right1, windowWidth / 2.0f - 500.0f, windowHeight / 2.0f + 50.0f);
                                                                Right2.setString(L"PRAWO (" + Keyboard::getDescription(keyRight2) + L")");
                                                                centerText(Right2, windowWidth / 2.0f, windowHeight / 2.0f + 50.0f);
                                                                Right3.setString(L"PRAWO (" + Keyboard::getDescription(keyRight3) + L")");
                                                                centerText(Right3, windowWidth / 2.0f + 500.0f, windowHeight / 2.0f + 50.0f);
                                                            }
                                                            // klikniÍcie WYJåCIE
                                                            if (keyBindingsExit.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();
                                                                inKeyBindings = false;
                                                            }
                                                        }
                                                    }
                                                }

                                                // tworzenie okna do przypisywania klawiszy
                                                // inKeyBindings = false wraca do ustawieÒ

                                                // zmiana koloru po najechaniu myszkπ - ustawienia sterowania

                                                if (Jump1.getGlobalBounds().contains(mousePos)) Jump1.setFillColor(Color(0x008000FF));
                                                else Jump1.setFillColor(Color(0x6B8E23FF));

                                                if (Jump2.getGlobalBounds().contains(mousePos)) Jump2.setFillColor(Color(0x008000FF));
                                                else Jump2.setFillColor(Color(0x6B8E23FF));

                                                if (Jump3.getGlobalBounds().contains(mousePos)) Jump3.setFillColor(Color(0x008000FF));
                                                else Jump3.setFillColor(Color(0x6B8E23FF));

                                                if (Right1.getGlobalBounds().contains(mousePos)) Right1.setFillColor(Color(0x008000FF));
                                                else Right1.setFillColor(Color(0x6B8E23FF));

                                                if (Right2.getGlobalBounds().contains(mousePos)) Right2.setFillColor(Color(0x008000FF));
                                                else Right2.setFillColor(Color(0x6B8E23FF));

                                                if (Right3.getGlobalBounds().contains(mousePos)) Right3.setFillColor(Color(0x008000FF));
                                                else Right3.setFillColor(Color(0x6B8E23FF));

                                                if (Left1.getGlobalBounds().contains(mousePos)) Left1.setFillColor(Color(0x008000FF));
                                                else Left1.setFillColor(Color(0x6B8E23FF));

                                                if (Left2.getGlobalBounds().contains(mousePos)) Left2.setFillColor(Color(0x008000FF));
                                                else Left2.setFillColor(Color(0x6B8E23FF));

                                                if (Left3.getGlobalBounds().contains(mousePos)) Left3.setFillColor(Color(0x008000FF));
                                                else Left3.setFillColor(Color(0x6B8E23FF));

                                                if (Run1.getGlobalBounds().contains(mousePos)) Run1.setFillColor(Color(0x008000FF));
                                                else Run1.setFillColor(Color(0x6B8E23FF));

                                                if (Run2.getGlobalBounds().contains(mousePos)) Run2.setFillColor(Color(0x008000FF));
                                                else Run2.setFillColor(Color(0x6B8E23FF));

                                                if (Run3.getGlobalBounds().contains(mousePos)) Run3.setFillColor(Color(0x008000FF));
                                                else Run3.setFillColor(Color(0x6B8E23FF));

                                                if (keyBindingsReset.getGlobalBounds().contains(mousePos)) keyBindingsReset.setFillColor(Color(0x008000FF));
                                                else keyBindingsReset.setFillColor(Color(0x6B8E23FF));

                                                if (keyBindingsExit.getGlobalBounds().contains(mousePos)) keyBindingsExit.setFillColor(Color::Blue);
                                                else keyBindingsExit.setFillColor(Color(0x008080FF));

                                                window.clear(Color(0x808080FF));

                                                window.draw(keyBindingsTitle);
                                                window.draw(Jump1);
                                                window.draw(Jump2);
                                                window.draw(Jump3);
                                                window.draw(Right1);
                                                window.draw(Right2);
                                                window.draw(Right3);
                                                window.draw(Left1);
                                                window.draw(Left2);
                                                window.draw(Left3);
                                                window.draw(Run1);
                                                window.draw(Run2);
                                                window.draw(Run3);
                                                window.draw(keyBindingsReset);
                                                window.draw(keyBindingsExit);

                                                window.display();
                                            }
                                        }
                                        // klikniÍcie DèWI K
                                        if (volumeSettings.getGlobalBounds().contains(mousePos))
                                        {
                                            clickSound.play();

                                            bool inVolumeSettings = true;
                                            bool isDraggingMusic = false;
                                            bool isDraggingSfx = false;

                                            Vector2f startMusicPos = musicSliderButton.getPosition();
                                            Vector2f startSfxPos = sfxSliderButton.getPosition();

                                            while (window.isOpen() && inVolumeSettings)
                                            {
                                                sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

                                                while (const optional event = window.pollEvent())
                                                {
                                                    if (event->is<Event::Closed>())
                                                    {
                                                        clickSound.play();
                                                        bgMusic.stop();
                                                        jumpSound.setLooping(false);
                                                        jumpSound.stop();
                                                        inVolumeSettings = false;
                                                    }
                                                    else if (const auto* key = event->getIf<Event::KeyPressed>())
                                                    {
                                                        if (key->scancode == Keyboard::Scancode::Escape)
                                                        {
                                                            bgMusic.stop();
                                                            jumpSound.setLooping(false);
                                                            jumpSound.stop();
                                                            inVolumeSettings = false;
                                                        }
                                                        if (key->scancode == Keyboard::Scancode::F11) {
                                                            isFullscreen = !isFullscreen;
                                                            window.close();
                                                            if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                                                            else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                                                            window.setFramerateLimit(140);
                                                        }
                                                    }
                                                    else if (const auto* mouseRel = event->getIf<sf::Event::MouseButtonReleased>())
                                                    {
                                                        if (mouseRel->button == sf::Mouse::Button::Left)
                                                        {
                                                            isDraggingMusic = false;
                                                            isDraggingSfx = false;
                                                        }
                                                    }
                                                    // klikniÍcie |> lub || lub ZRESETUJ
                                                    if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                                    {
                                                        if (mouseBtn->button == Mouse::Button::Left)
                                                        {
                                                            if (musicSliderButton.getGlobalBounds().contains(mousePos) || musicSliderBg.getGlobalBounds().contains(mousePos))
                                                            {
                                                                isDraggingMusic = true;
                                                            }
                                                            if (sfxSliderButton.getGlobalBounds().contains(mousePos) || sfxSliderBg.getGlobalBounds().contains(mousePos))
                                                            {
                                                                isDraggingSfx = true;
                                                            }
                                                            if (musicStart.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();
                                                                jumpSound.setLooping(false);
                                                                jumpSound.stop();
                                                                bgMusic.play();
                                                            }
                                                            if (musicStop.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();
                                                                bgMusic.stop();
                                                            }
                                                            if (sfxStart.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();
                                                                bgMusic.stop();
                                                                jumpSound.setLooping(true);
                                                                jumpSound.play();
                                                            }
                                                            if (sfxStop.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();
                                                                jumpSound.stop();
                                                                jumpSound.setLooping(false);
                                                            }
                                                            if (soundReset.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();

                                                                musicSliderButton.setPosition(startMusicPos);
                                                                sfxSliderButton.setPosition(startSfxPos);

                                                                float musicleft = musicSliderBg.getPosition().x - 200.0f;
                                                                float percentMusic = (startMusicPos.x - musicleft) / 400.0f;

                                                                bgMusic.setVolume(percentMusic * volumeMusicScale);

                                                                float sfxleft = sfxSliderBg.getPosition().x - 200.0f;
                                                                float percentSfx = (startSfxPos.x - sfxleft) / 400.0f;

                                                                jumpSound.setVolume(percentSfx * volumeSfxScale);
                                                                clickSound.setVolume(percentSfx * volumeSfxScale);
                                                                gameOverSound.setVolume(percentSfx * volumeSfxScale * 1.5f);

                                                                bgMusic.stop();
                                                                jumpSound.setLooping(false);
                                                                jumpSound.stop();
                                                            }
                                                            if (mouseBtn->button == Mouse::Button::Left)
                                                            {
                                                                if (volumeSettingsExit.getGlobalBounds().contains(mousePos))
                                                                {
                                                                    clickSound.play();
                                                                    bgMusic.stop();
                                                                    jumpSound.setLooping(false);
                                                                    jumpSound.stop();
                                                                    inVolumeSettings = false;
                                                                }
                                                            }
                                                        }
                                                    }
                                                    if (isDraggingMusic)
                                                    {
                                                        float musicleft = musicSliderBg.getPosition().x - 200.0f;
                                                        float musicright = musicSliderBg.getPosition().x + 200.0f;

                                                        float newX = mousePos.x;
                                                        if (newX < musicleft) newX = musicleft;
                                                        if (newX > musicright) newX = musicright;

                                                        musicSliderButton.setPosition({ newX, musicSliderBg.getPosition().y });

                                                        float percent = (newX - musicleft) / 400.0f; // 400.0f do d≥ugoúÊ paska

                                                        bgMusic.setVolume(percent * volumeMusicScale);
                                                    }
                                                    if (isDraggingSfx)
                                                    {
                                                        float sfxleft = sfxSliderBg.getPosition().x - 200.0f;
                                                        float sfxright = sfxSliderBg.getPosition().x + 200.0f;

                                                        float newX = mousePos.x;
                                                        if (newX < sfxleft) newX = sfxleft;
                                                        if (newX > sfxright) newX = sfxright;

                                                        sfxSliderButton.setPosition({ newX, sfxSliderBg.getPosition().y });

                                                        float percent = (newX - sfxleft) / 400.0f;

                                                        jumpSound.setVolume(percent * volumeSfxScale);
                                                        clickSound.setVolume(percent * volumeSfxScale);
                                                        gameOverSound.setVolume(percent * volumeSfxScale * 1.5f);
                                                    }
                                                }

                                                // zmiana koloru / gruboúci kontur po najechaniu myszkπ - ustawienia g≥oúnoúci

                                                if (musicStart.getGlobalBounds().contains(mousePos)) musicStart.setOutlineThickness(3.0f);
                                                else musicStart.setOutlineThickness(1.5f);

                                                if (musicStop.getGlobalBounds().contains(mousePos)) musicStop.setOutlineThickness(3.0f);
                                                else musicStop.setOutlineThickness(1.5f);

                                                if (sfxStart.getGlobalBounds().contains(mousePos)) sfxStart.setOutlineThickness(3.0f);
                                                else sfxStart.setOutlineThickness(1.5f);

                                                if (sfxStop.getGlobalBounds().contains(mousePos)) sfxStop.setOutlineThickness(3.0f);
                                                else sfxStop.setOutlineThickness(1.5f);

                                                if (soundReset.getGlobalBounds().contains(mousePos)) soundReset.setOutlineThickness(3.0f);
                                                else soundReset.setOutlineThickness(1.5f);

                                                if (volumeSettingsExit.getGlobalBounds().contains(mousePos)) volumeSettingsExit.setFillColor(Color::Blue);
                                                else volumeSettingsExit.setFillColor(Color(0x008080FF));

                                                // tworzenie okna do ustawieÒ g≥oúnoúci

                                                window.clear(Color(0x808080FF));

                                                window.draw(volumeSettingsTitle);

                                                window.draw(musicSettings);
                                                window.draw(musicSliderBg);
                                                window.draw(musicSliderButton);
                                                window.draw(musicStart);
                                                window.draw(musicStop);

                                                window.draw(sfxSettings);
                                                window.draw(sfxSliderBg);
                                                window.draw(sfxSliderButton);
                                                window.draw(sfxStart);
                                                window.draw(sfxStop);

                                                window.draw(soundReset);
                                                window.draw(volumeSettingsExit);

                                                window.display();
                                            }
                                        }
                                        // klikniÍcie GRAFIKA
                                        if (graphicsSettings.getGlobalBounds().contains(mousePos))
                                        {
                                            clickSound.play();
                                            bool inGraphicsSettings = true;

                                            while (window.isOpen() && inGraphicsSettings)
                                            {
                                                sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

                                                while (const optional event = window.pollEvent())
                                                {
                                                    if (event->is<Event::Closed>())
                                                    {
                                                        clickSound.play();
                                                        inGraphicsSettings = false;
                                                    }
                                                    else if (const auto* key = event->getIf<Event::KeyPressed>())
                                                    {
                                                        if (key->scancode == Keyboard::Scancode::Escape)
                                                        {
                                                            inGraphicsSettings = false;
                                                        }
                                                        if (key->scancode == Keyboard::Scancode::F11) {
                                                            isFullscreen = !isFullscreen;
                                                            window.close();
                                                            if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                                                            else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                                                            window.setFramerateLimit(140);
                                                        }
                                                    }
                                                    if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                                    {
                                                        if (mouseBtn->button == Mouse::Button::Left)
                                                        {
                                                            // klikniÍcie PE£NY EKRAN
                                                            if (fullscreenSettings.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();

                                                                if (!isFullscreen)
                                                                {
                                                                    isFullscreen = true;
                                                                    window.close();
                                                                    window.create(VideoMode::getDesktopMode(), "Projekt Podstawy Informatyki", Style::Default, sf::State::Fullscreen);
                                                                    window.setFramerateLimit(140);
                                                                }
                                                                else {}
                                                            }
                                                            // klikniÍcie TRYB OKIENKOWY
                                                            if (windowedSettings.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();

                                                                if (isFullscreen)
                                                                {
                                                                    isFullscreen = false;
                                                                    window.close();
                                                                    window.create(VideoMode({ windowWidth, windowHeight }), "Projekt Podstawy Informatyki", Style::Default, sf::State::Windowed);
                                                                    window.setFramerateLimit(140);
                                                                }
                                                                else {}
                                                            }
                                                            // kilkniÍcie WYJåCIE
                                                            if (graphicsSettingsExit.getGlobalBounds().contains(mousePos))
                                                            {
                                                                clickSound.play();
                                                                inGraphicsSettings = false;
                                                            }
                                                        }
                                                    }
                                                }

                                                if (fullscreenSettings.getGlobalBounds().contains(mousePos)) fullscreenSettings.setFillColor(Color(0x800080FF));
                                                else fullscreenSettings.setFillColor(Color(0x9370DBFF));

                                                if (windowedSettings.getGlobalBounds().contains(mousePos)) windowedSettings.setFillColor(Color(0x800080FF));
                                                else windowedSettings.setFillColor(Color(0x9370DBFF));

                                                if (graphicsSettingsExit.getGlobalBounds().contains(mousePos)) graphicsSettingsExit.setFillColor(Color::Blue);
                                                else graphicsSettingsExit.setFillColor(Color(0x008080FF));

                                                window.clear(Color(0x808080FF));

                                                window.draw(graphicsSettingsTitle);
                                                window.draw(fullscreenSettings);
                                                window.draw(windowedSettings);
                                                window.draw(graphicsSettingsExit);

                                                window.display();
                                            }
                                        }
                                    }
                                }
                                if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
                                {
                                    if (mouseBtn->button == Mouse::Button::Left)
                                    {
                                        if (SettingsExit.getGlobalBounds().contains(mousePos))
                                        {
                                            clickSound.play();
                                            inSettings = false;
                                        }
                                    }
                                }
                                else if (const auto* key = event->getIf<Event::KeyPressed>())
                                {
                                    if (key->scancode == Keyboard::Scancode::Escape)
                                    {
                                        inSettings = false;
                                    }
                                    if (key->scancode == Keyboard::Scancode::F11) {
                                        isFullscreen = !isFullscreen;
                                        window.close();
                                        if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "Projekt PI", sf::Style::Default, sf::State::Fullscreen);
                                        else window.create(sf::VideoMode({ windowWidth, windowHeight }), "Projekt PI", sf::Style::Default, sf::State::Windowed);
                                        window.setFramerateLimit(140);
                                    }
                                }

                            }

                            // zmiana koloru po najechaniu myszkπ - ustawienia

                            if (graphicsSettings.getGlobalBounds().contains(mousePos)) graphicsSettings.setFillColor(Color::Blue);
                            else graphicsSettings.setFillColor(Color(0x008080FF));

                            if (keyBindings.getGlobalBounds().contains(mousePos)) keyBindings.setFillColor(Color::Blue);
                            else keyBindings.setFillColor(Color(0x008080FF));

                            if (volumeSettings.getGlobalBounds().contains(mousePos)) volumeSettings.setFillColor(Color::Blue);
                            else volumeSettings.setFillColor(Color(0x008080FF));

                            if (SettingsExit.getGlobalBounds().contains(mousePos)) SettingsExit.setFillColor(Color(0x8B4000FF));
                            else SettingsExit.setFillColor(Color(0xFFA500FF));

                            // tworzenie okna ustawieÒ

                            window.clear(Color(0x808080FF));

                            window.draw(settingsTitle);
                            window.draw(graphicsSettings);
                            window.draw(keyBindings);
                            window.draw(volumeSettings);
                            window.draw(SettingsExit);

                            window.display();
                        }
                    }

                    //***********

                    if (menuExit.getGlobalBounds().contains(mousePos)) {
                        // Opcjonalnie: ma≥a pauza, øeby düwiÍk zdπøy≥ wybrzmieÊ przed zamkniÍciem                                             
                        sf::sleep(sf::milliseconds(150));
                        window.close();
                        return 0;
                    }
                }
            }
        }




        // Efekty najechania (Hover) - robimy poza pÍtlπ zdarzeÒ, øeby dzia≥o siÍ ca≥y czas
        if (menuStart.getGlobalBounds().contains(mousePos)) menuStart.setFillColor(Color(0x8B4000FF));              // zmienia kolor po najechaniu myszkπ
        else menuStart.setFillColor(Color(0xFFA500FF));

        if (menuSettings.getGlobalBounds().contains(mousePos)) menuSettings.setFillColor(Color(0x8B4000FF));        //*
        else menuSettings.setFillColor(Color(0xFFA500FF));                                                           //*            

        if (menuExit.getGlobalBounds().contains(mousePos)) menuExit.setFillColor(Color(0x8B4000FF));
        else menuExit.setFillColor(Color(0xFFA500FF));
        //
        window.clear(sf::Color(0x808080FF)); // Ciemne t≥o              //  kolor ekranu startowego
        window.draw(menuTitle);                                         //
        window.draw(menuStart);
        window.draw(menuSettings);
        window.draw(menuExit);
        window.display();
    }
    // ----------------------------

    // Jeúli okno zosta≥o zamkniÍte w menu, nie idü dalej
    if (!window.isOpen()) return 0;



    // --- ZMIENNE ROZGRYWKI ---
    bool gameStarted = false;
    bool isPaused = false;
    bool isLevelUp = false;
    bool isGameOver = false;
    bool bossSpawned = false; // Czy boss juø siÍ pojawi≥?

    // UI BOSSA (Pasek zdrowia)
    sf::RectangleShape bossBarBg(sf::Vector2f(800.f, 30.f));
    bossBarBg.setFillColor(sf::Color(50, 0, 0)); // Ciemnoczerwone t≥o
    bossBarBg.setOutlineColor(sf::Color::White);
    bossBarBg.setOutlineThickness(2.f);
    bossBarBg.setOrigin(sf::Vector2f(400.f, 0.f)); // Punkt zaczepienia na úrodku
    bossBarBg.setPosition(sf::Vector2f(windowWidth / 2.f, 100.f)); // Na gÛrze ekranu

    sf::RectangleShape bossBarFill(sf::Vector2f(800.f, 30.f));
    bossBarFill.setFillColor(sf::Color::Red); // Jasnoczerwone øycie
    bossBarFill.setOrigin(sf::Vector2f(400.f, 0.f));
    bossBarFill.setPosition(sf::Vector2f((windowWidth / 2.f) - 400.f, 100.f));

    sf::Text bossNameText(font);
    bossNameText.setString("WIELKI NIETOPERZ");
    bossNameText.setCharacterSize(24);
    bossNameText.setFillColor(sf::Color::White);
    centerText(bossNameText, windowWidth / 2.f, 75.f); // Nad paskiem


    // --- GRACZ JAKO SPRITE (ZMIANA) ---
    sf::Texture idleTexture, walkTexture, runTexture, jumpTexture, fallTexture, hurtTexture, deathTexture, runToIdleTexture, turnWalkTexture, turnRunTexture;

    // Wczytujemy wszystkie 3 pliki
    if (!idleTexture.loadFromFile("animacje/idle.png")) return -1;                                                       //
    if (!walkTexture.loadFromFile("animacje/walk.png")) return -1;                                                       //
    if (!runTexture.loadFromFile("animacje/run.png")) return -1;                                                         //
    if (!jumpTexture.loadFromFile("animacje/jump.png")) return -1;                                                       //
    if (!fallTexture.loadFromFile("animacje/fall.png")) return -1;                                                       //  wczytywanie animacji postaci
    if (!hurtTexture.loadFromFile("animacje/hurt.png")) return -1;                                                       //
    if (!deathTexture.loadFromFile("animacje/death.png")) return -1;                                                     //
    if (!runToIdleTexture.loadFromFile("animacje/run_to_idle.png")) return -1;                                           //
    if (!turnWalkTexture.loadFromFile("animacje/turn_walk.png")) return -1;                                              //
    if (!turnRunTexture.loadFromFile("animacje/turn_run.png")) return -1;                                                //

    sf::Sprite player(idleTexture); // Startujemy z idle

    // POPRAWKA: Uøywamy standardowego konstruktora IntRect (nawiasy okrπg≥e)
    player.setTextureRect(sf::IntRect({ 0, 0 }, { 128, 128 }));

    // POPRAWKA: Podajemy po prostu dwie liczby (x, y) zamiast tworzyÊ obiekt sf::Vector2f
    player.setOrigin(sf::Vector2f(64.f, 64.f));
    player.setScale(sf::Vector2f(2.0f, 2.0f));

    // Pozycja startowa (taka sama jak wczeúniej)
    sf::Vector2f startPos = { mapWidth / 2.f, windowHeight - 100.f };
    player.setPosition(startPos);

    // --- HITBOX FIZYCZNY ---
    // To jest "prawdziwe cia≥o" gracza (ma≥e kÛ≥ko)
    sf::CircleShape hitbox(15.f); // PromieÒ 15 (úrednica 30) - ma≥y hitbox
    hitbox.setFillColor(sf::Color::Blue); // Kolor bez znaczenia, bo i tak nie bÍdziemy go rysowaÊ                          //rozmiar hitboxa
    hitbox.setOrigin(sf::Vector2f(15.f, 15.f)); // årodek kÛ≥ka                                                             
    hitbox.setPosition(startPos);
    // -------------------------------

    // Zmienne do animacji
    int currentAnimFrame = 0;
    float animTimer = 0.0f;
    float frameDuration = 0.1f;
    bool isHurt = false;
    bool isDying = false;
    bool isBraking = false;
    bool isTurning = false;
    // ----------------------------------

    // Zmienne dynamiczne
    float currentHp = 10;
    int facingDir = 1;
    PlayerStats pStats;

    // EKWIPUNEK
    std::vector<ActiveWeapon> weaponInventory;          // max 3 bronie
    std::vector<ActivePassive> passiveInventory;        // max 3 przedmioty

    int level = 1;
    int exp = 0;
    int nextExp = 50;

    std::vector<Enemy> enemies;
    std::vector<Projectile> projectiles;
    std::vector<DamageZone> zones;
    std::vector<ExpOrb> expOrbs;
    std::vector<UpgradeCard> upgradeCards;

    sf::Clock dtClock;
    sf::Clock spawnClock;
    sf::Clock regenClock; // Zegar regeneracji
    float gameTime = 0.f;

    // Obiekt Ground (Pod≥oga) - øeby unikaÊ b≥Ídu z poprzedniej tury
    sf::RectangleShape ground({ mapWidth, 100.f });
    ground.setFillColor(sf::Color::Transparent);
    ground.setPosition({ 0.f, windowHeight - 80.f });

    float baseGravity = 2500.f;
    float jumpForce = -1100.f;
    bool onGround = false;
    sf::Vector2f velocity(0.f, 0.f);

    int nextEnemyId = 0;


    // --- P TLA WYBORU BRONI (POPRAWIONA) ---
    while (window.isOpen() && !gameStarted) {
        sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window); // Update pozycji myszki

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->scancode == sf::Keyboard::Scancode::F11) {
                    isFullscreen = !isFullscreen;
                    window.close();
                    if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "VS Clone", sf::Style::Default, sf::State::Fullscreen);        // nazwa na pasku zadaÒ
                    else window.create(sf::VideoMode({ windowWidth, windowHeight }), "VS Clone", sf::Style::Default, sf::State::Windowed);          //
                    window.setFramerateLimit(140);
                }
            }
            // ZMIANA: Obs≥uga klikniÍcia jako ZDARZENIE wewnπtrz pÍtli pollEvent
            else if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {                        //
                if (mouseBtn->button == sf::Mouse::Button::Left) {                                                  //      Funkcja dla klikniÍcia myszkπ 
                    for (auto& btn : weaponButtons) {                                                               //
                        if (btn.shape.getGlobalBounds().contains(mousePos)) {
                            // Tutaj wybieramy broÒ
                            weaponInventory.clear();
                            weaponInventory.push_back({ weaponDB[btn.id], 1, 0.f });
                            gameStarted = true;
                            bgMusic.play();
                            pStats = recalculateStats(passiveInventory);
                            currentHp = (float)pStats.maxHp;
                            dtClock.restart();
                        }
                    }
                }
            }
        }

        window.clear(sf::Color::Black);
        window.draw(title);

        for (auto& btn : weaponButtons) {
            // Logika Hover (podúwietlanie) - moøe zostaÊ poza pÍtlπ zdarzeÒ
            if (btn.shape.getGlobalBounds().contains(mousePos)) {
                btn.shape.setOutlineThickness(4.f);
            }
            else {
                btn.shape.setOutlineThickness(2.f);
            }
            window.draw(btn.shape);
            window.draw(btn.text);
        }
        window.display();
    }

    if (!window.isOpen()) return 0;

    // --- G£”WNA P TLA ---
    while (window.isOpen()) {
        float dt = dtClock.restart().asSeconds();
        sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->scancode == sf::Keyboard::Scancode::F11) {
                    isFullscreen = !isFullscreen;
                    window.close();
                    if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "VS Clone", sf::Style::Default, sf::State::Fullscreen);
                    else window.create(sf::VideoMode({ windowWidth, windowHeight }), "VS Clone", sf::Style::Default, sf::State::Windowed);
                    window.setFramerateLimit(140);
                    dtClock.restart();
                }
                if (key->scancode == sf::Keyboard::Scancode::Escape && !isGameOver) {
                    isPaused = !isPaused;
                    if (!isPaused) dtClock.restart();
                }
            }
        }

        if (isGameOver) {
            // Logika przycisku RESTART (teø warto zmieniÊ na Event w przysz≥oúci, ale tu zadzia≥a teø click hold)
            if (restartButton.getGlobalBounds().contains(mousePos)) {
                restartButton.setFillColor(sf::Color::Green);
                if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {

                    // --- PE£NY RESET GRY ---
                    gameStarted = false;
                    isGameOver = false;
                    isDying = false;
                    isHurt = false;
                    isBraking = false;
                    isTurning = false;

                    // 1. Reset postÍpu
                    level = 1;
                    exp = 0;
                    nextExp = 50;
                    gameTime = 0.f;       // WAØNE: Reset czasu, øeby wrogowie nie byli od razu trudni

                    // 2. Czyszczenie wektorÛw
                    enemies.clear();
                    projectiles.clear();
                    zones.clear();
                    expOrbs.clear();
                    upgradeCards.clear();
                    weaponInventory.clear();
                    passiveInventory.clear();

                    // 3. Reset statystyk i gracza
                    pStats = recalculateStats(passiveInventory);
                    currentHp = (float)pStats.maxHp;
                    player.setPosition(startPos);
                    hitbox.setPosition(startPos);

                    // 4. Reset zegarÛw
                    spawnClock.restart();
                    regenClock.restart();

                    // --- POWR”T DO MENU WYBORU BRONI (ZMODYFIKOWANA P TLA) ---
                    while (window.isOpen() && !gameStarted) {
                        sf::Vector2f mp = (sf::Vector2f)sf::Mouse::getPosition(window);
                        while (const std::optional event = window.pollEvent()) {
                            if (event->is<sf::Event::Closed>()) { window.close(); return 0; }

                            // Znowu uøywamy EVENT, øeby nie klika≥o podwÛjnie
                            else if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                                if (mouseBtn->button == sf::Mouse::Button::Left) {
                                    for (auto& btn : weaponButtons) {
                                        if (btn.shape.getGlobalBounds().contains(mp)) {
                                            weaponInventory.push_back({ weaponDB[btn.id], 1, 0.f });
                                            gameStarted = true;
                                            bgMusic.play();
                                            pStats = recalculateStats(passiveInventory);
                                            currentHp = (float)pStats.maxHp;
                                            dtClock.restart();
                                        }
                                    }
                                }
                            }
                        }

                        window.clear(sf::Color::Black);
                        window.draw(title);

                        for (auto& btn : weaponButtons) {
                            if (btn.shape.getGlobalBounds().contains(mp)) btn.shape.setOutlineThickness(4.f);
                            else btn.shape.setOutlineThickness(2.f);

                            window.draw(btn.shape);
                            window.draw(btn.text);
                        }
                        window.display();
                    }
                }
            }
            else restartButton.setFillColor(sf::Color::White);
        }
        else if (isLevelUp) {
            // LOGIKA WYBORU KARTY
            // Tutaj click hold jest mniej groüny, bo pauzuje grÍ, ale dla spÛjnoúci moøna teø uøyÊ eventÛw
            for (size_t i = 0; i < upgradeCards.size(); ++i) {
                if (upgradeCards[i].shape.getGlobalBounds().contains(mousePos)) {
                    upgradeCards[i].shape.setOutlineColor(sf::Color::Yellow);
                    if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                        int idx = upgradeCards[i].index;
                        int type = upgradeCards[i].type;

                        if (type == 0) { // BroÒ
                            bool found = false;
                            for (auto& w : weaponInventory) {
                                if (w.def.id == weaponDB[idx].id) { w.level++; found = true; break; }
                            }
                            if (!found) weaponInventory.push_back({ weaponDB[idx], 1, 0.f });
                        }
                        else { // Pasywka
                            bool found = false;
                            for (auto& p : passiveInventory) {
                                if (p.def.id == passiveDB[idx].id) { p.level++; found = true; break; }
                            }
                            if (!found) passiveInventory.push_back({ passiveDB[idx], 1 });

                            // Przelicz statystyki po zdobyciu pasywki
                            float hpPercentage = currentHp / (float)pStats.maxHp;
                            pStats = recalculateStats(passiveInventory);
                            currentHp = hpPercentage * (float)pStats.maxHp;
                        }

                        isLevelUp = false;
                        dtClock.restart();
                    }
                }
                else upgradeCards[i].shape.setOutlineColor(sf::Color::White);
            }
        }
        else if (isPaused) {
            // Obs≥uga przycisku Restart w menu pauzy
            if (pauseRestart.getGlobalBounds().contains(mousePos)) {
                pauseRestart.setFillColor(sf::Color::Red);

                // Uøywamy isButtonPressed zamiast Eventu dla uproszczenia w tym miejscu (jak w Game Over)
                if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {

                    // --- KOD RESETU (taki sam jak w Game Over) ---
                    gameStarted = false;
                    isGameOver = false;
                    isPaused = false; // Waøne: odznaczamy pauzÍ

                    level = 1;
                    exp = 0;
                    nextExp = 50;
                    gameTime = 0.f;

                    enemies.clear();
                    projectiles.clear();
                    zones.clear();
                    expOrbs.clear();
                    upgradeCards.clear();
                    weaponInventory.clear();
                    passiveInventory.clear();

                    pStats = recalculateStats(passiveInventory);
                    currentHp = (float)pStats.maxHp;
                    player.setPosition(startPos);
                    hitbox.setPosition(startPos);

                    spawnClock.restart();
                    regenClock.restart();

                    // P TLA WYBORU BRONI
                    while (window.isOpen() && !gameStarted) {
                        sf::Vector2f mp = (sf::Vector2f)sf::Mouse::getPosition(window);
                        while (const std::optional event = window.pollEvent()) {
                            if (event->is<sf::Event::Closed>()) { window.close(); return 0; }
                            else if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {
                                if (mouseBtn->button == sf::Mouse::Button::Left) {
                                    for (auto& btn : weaponButtons) {
                                        if (btn.shape.getGlobalBounds().contains(mp)) {
                                            weaponInventory.push_back({ weaponDB[btn.id], 1, 0.f });
                                            gameStarted = true;
                                            bgMusic.play();
                                            pStats = recalculateStats(passiveInventory);
                                            currentHp = (float)pStats.maxHp;
                                            dtClock.restart();
                                        }
                                    }
                                }
                            }
                        }
                        window.clear(sf::Color::Black);
                        window.draw(title);
                        for (auto& btn : weaponButtons) {
                            if (btn.shape.getGlobalBounds().contains(mp)) btn.shape.setOutlineThickness(4.f);
                            else btn.shape.setOutlineThickness(2.f);
                            window.draw(btn.shape);
                            window.draw(btn.text);
                        }
                        window.display();
                    }
                }
            }
            else {
                pauseRestart.setFillColor(sf::Color::White);
            }
        }
        else if (!isPaused) {
            gameTime += dt;

            // --- AKTUALIZACJA ANIMACJI (PRIORYTETY: åMIER∆ > OBRAØENIA > SKOK > OBR”T > HAMOWANIE > RUCH) ---
            animTimer += dt;
            float speedAbs = std::abs(velocity.x);
            frameDuration = 0.1f;

            sf::Texture* nextTexture = &idleTexture;
            int maxFrames = 10;
            bool loopAnimation = true;

            // 0a. DETEKCJA HAMOWANIA
            if (onGround && !isBraking && !isTurning && !isHurt && !isDying && speedAbs < 10.f && &player.getTexture() == &runTexture) {
                isBraking = true;
            }
            if (speedAbs > 10.f || !onGround || isHurt || isDying || isTurning) {
                isBraking = false;
            }

            // 1. WYB”R STANU
            if (isDying) {
                nextTexture = &deathTexture; maxFrames = 23; loopAnimation = false;
            }
            else if (isHurt) {
                nextTexture = &hurtTexture; maxFrames = 6; loopAnimation = false;
            }
            else if (!onGround) {
                if (velocity.y < 0) { nextTexture = &jumpTexture; maxFrames = 6; }
                else { nextTexture = &fallTexture; maxFrames = 4; }
            }
            else {
                // ZIEMIA
                if (isTurning) {
                    // WybÛr: obrÛt z biegu czy chodu?
                    // Sprawdzamy czy prÍdkoúÊ by≥a duøa (sprint)
                    if (speedAbs > pStats.moveSpeed * 1.1f) nextTexture = &turnRunTexture;
                    else nextTexture = &turnWalkTexture;

                    maxFrames = 4; // ObrÛt ma 4 klatki
                    loopAnimation = false;
                    frameDuration = 0.08f; // ObrÛt jest doúÊ szybki
                }
                else if (isBraking) {
                    nextTexture = &runToIdleTexture; maxFrames = 7; loopAnimation = false;
                }
                else if (speedAbs < 10.f) {
                    nextTexture = &idleTexture;
                }
                else if (speedAbs > pStats.moveSpeed * 1.1f) {
                    nextTexture = &runTexture; frameDuration = 0.06f;
                }
                else {
                    nextTexture = &walkTexture;
                }
            }

            // 2. ZMIANA TEKSTURY
            if (&player.getTexture() != nextTexture) {
                player.setTexture(*nextTexture);
                currentAnimFrame = 0;
                animTimer = 0.f;
                player.setTextureRect(sf::IntRect({ 0, 0 }, { 128, 128 }));
            }

            // 3. PRZEWIJANIE KLATEK
            if (animTimer >= frameDuration) {
                animTimer = 0.f;
                currentAnimFrame++;

                if (currentAnimFrame >= maxFrames) {
                    if (loopAnimation) {
                        currentAnimFrame = 0;
                    }
                    else {
                        // KONIEC ANIMACJI JEDNORAZOWYCH
                        if (isDying) {
                            currentAnimFrame = maxFrames - 1; isGameOver = true;
                        }
                        else if (isHurt) {
                            isHurt = false; currentAnimFrame = 0;
                        }
                        else if (isBraking) {
                            isBraking = false; currentAnimFrame = 0;
                        }
                        else if (isTurning) {
                            // WAØNE: Koniec obrotu -> odwracamy postaÊ
                            isTurning = false;
                            facingDir = -facingDir; // ZmieÒ kierunek na przeciwny
                            currentAnimFrame = 0;
                        }
                    }
                }
                player.setTextureRect(sf::IntRect({ currentAnimFrame * 128, 0 }, { 128, 128 }));
            }

            // 4. OBRACANIE
            if (!isDying) {
                // Jeúli trwa animacja obrotu, NIE odwracamy jeszcze skali (grafika obrotu robi to wizualnie)
                // Odwracamy dopiero po zakoÒczeniu (gdy zmieni siÍ facingDir w bloku wyøej)
                if (facingDir == -1) player.setScale(sf::Vector2f(-2.0f, 2.0f));
                else player.setScale(sf::Vector2f(2.0f, 2.0f));
            }
            // ----------------------------------------------------

            // 1. RUCH
            velocity.x = 0.f; // Domyúlnie zerujemy prÍdkoúÊ

            // ZMIANA: Pozwalamy na sterowanie TYLKO jeúli postaÊ NIE umiera
            if (!isDying) {
                float currentSpeed = pStats.moveSpeed;

                // Sprint
                if (Keyboard::isKeyPressed(keyRun1) || Keyboard::isKeyPressed(keyRun2) || Keyboard::isKeyPressed(keyRun3))
                {
                    currentSpeed *= 1.7f;
                }

                // --- STEROWANIE LEWO (A) ---
                if (Keyboard::isKeyPressed(keyLeft1) || Keyboard::isKeyPressed(keyLeft2) || Keyboard::isKeyPressed(keyLeft3))
                {
                    velocity.x -= currentSpeed;

                    // Jeúli patrzymy w PRAWO (1) i nie robimy jeszcze obrotu -> ZACZNIJ OBR”T
                    if (facingDir == 1 && !isTurning && onGround) {
                        isTurning = true;
                        isBraking = false; // ObrÛt przerywa hamowanie
                        currentAnimFrame = 0;
                        animTimer = 0.f;
                    }
                    // Jeúli nie jesteúmy na ziemi lub juø siÍ obrÛciliúmy -> po prostu ustaw kierunek
                    else if (!isTurning) {
                        facingDir = -1;
                    }
                }

                // --- STEROWANIE PRAWO (D) ---
                if (Keyboard::isKeyPressed(keyRight1) || Keyboard::isKeyPressed(keyRight2) || Keyboard::isKeyPressed(keyRight3))
                {
                    velocity.x += currentSpeed;

                    // Jeúli patrzymy w LEWO (-1) i nie robimy jeszcze obrotu -> ZACZNIJ OBR”T
                    if (facingDir == -1 && !isTurning && onGround) {
                        isTurning = true;
                        isBraking = false;
                        currentAnimFrame = 0;
                        animTimer = 0.f;
                    }
                    else if (!isTurning) {
                        facingDir = 1;
                    }
                }

                // Skok (W, Strza≥ka w gÛrÍ LUB Spacja)
                if ((sf::Keyboard::isKeyPressed(keyJump1) || sf::Keyboard::isKeyPressed(keyJump2) || sf::Keyboard::isKeyPressed(keyJump3)) && onGround)
                {

                    velocity.y = jumpForce;
                    onGround = false;
                    isTurning = false; // Skok anuluje animacjÍ obrotu

                    jumpSound.play();
                }
            }

            // Grawitacja dzia≥a zawsze (nawet jak umiera, musi spaúÊ na ziemiÍ)
            velocity.y += baseGravity * dt;

            hitbox.move(velocity * dt);

            // Ograniczenia mapy
            if (hitbox.getPosition().x < 30) hitbox.setPosition({ 30, hitbox.getPosition().y });
            if (hitbox.getPosition().x > mapWidth - 30) hitbox.setPosition({ mapWidth - 30, hitbox.getPosition().y });

            if (hitbox.getPosition().y + 30.f > ground.getPosition().y) {
                hitbox.setPosition({ hitbox.getPosition().x, ground.getPosition().y - 30.f });
                velocity.y = 0.f;
                onGround = true;
            }
            else onGround = false;

            player.setPosition(hitbox.getPosition());

            // --- SPAWNOWANIE PRZECIWNIK”W (BALANS + DYSTANS) ---
            if (spawnClock.getElapsedTime().asSeconds() > 1.0f - (gameTime * 0.005f)) {

                // --- KOD BOSSA (NOWE) ---
                // 360 sekund = 6 minut
                if (!bossSpawned && gameTime >= 360.f) {
                    Enemy boss;
                    boss.id = nextEnemyId++;
                    boss.hp = 100.0f;
                    boss.maxHp = 100.0f;
                    boss.isBoss = true; // To jest boss!

                    // Wyglπd bossa (Duøy i inny kolor)
                    boss.shape.setRadius(60.f);
                    boss.shape.setPointCount(5); // Np. piÍciokπt, øeby siÍ rÛøni≥
                    boss.shape.setFillColor(sf::Color(139, 0, 0)); // Ciemny czerwieÒ
                    boss.shape.setOutlineColor(sf::Color::Yellow);
                    boss.shape.setOutlineThickness(3.f);
                    boss.shape.setOrigin(sf::Vector2f(60.f, 60.f));

                    // Pozycja (daleko od gracza)
                    float angle = (float)(rand() % 360);
                    float dist = 800.f; // Zawsze 800px od gracza
                    sf::Vector2f spawnOff(std::cos(angle) * dist, std::sin(angle) * dist);
                    boss.shape.setPosition(player.getPosition() + spawnOff);

                    enemies.push_back(boss);
                    bossSpawned = true; // Zablokuj ponowne spawnowanie

                    // Nie spawnuj zwyk≥ego wroga w tej samej klatce co bossa
                    spawnClock.restart();
                    continue; // Przejdü do nastÍpnej klatki pÍtli
                }
                // ------------------------

                Enemy e;
                e.id = nextEnemyId++;

                // 1. WYB”R TYPU PRZECIWNIKA (Zaleønie od czasu)
                int enemyType = 0; // 0 = S≥aby (Domyúlny)
                int randVal = rand() % 100; // Losowa liczba 0-99

                if (gameTime >= 240.f) { // Po 4 minutach
                    // 50% S≥aby, 30% åredni, 20% Silny
                    if (randVal < 50) enemyType = 0;
                    else if (randVal < 80) enemyType = 1;
                    else enemyType = 2;
                }
                else if (gameTime >= 120.f) { // Po 2 minutach
                    // 60% S≥aby, 40% åredni
                    if (randVal < 60) enemyType = 0;
                    else enemyType = 1;
                }
                else {
                    // Tylko s≥aby
                    enemyType = 0;
                }

                // 2. USTAWIENIE STATYSTYK
                if (enemyType == 0) { // Podstawowy
                    e.hp = 1.0f; e.maxHp = 1.0f;
                    e.shape.setRadius(25.f);
                    e.shape.setFillColor(sf::Color::Magenta); // RÛøowy
                }
                else if (enemyType == 1) { // åredni (od 2 min)
                    e.hp = 5.0f; e.maxHp = 5.0f;
                    e.shape.setRadius(30.f);
                    e.shape.setFillColor(sf::Color::Cyan); // B≥Íkitny
                    // Opcjonalnie wolniejszy? (logika prÍdkoúci jest niøej w pÍtli ruchu)
                }
                else if (enemyType == 2) { // Twardy (od 4 min)
                    e.hp = 10.0f; e.maxHp = 10.0f;
                    e.shape.setRadius(35.f);
                    e.shape.setFillColor(sf::Color::Red); // Czerwony
                }

                e.shape.setOrigin(sf::Vector2f(e.shape.getRadius(), e.shape.getRadius()));

                // 3. BEZPIECZNA POZYCJA (Nie za blisko gracza)
                sf::Vector2f spawnPos;
                float distToPlayer = 0.f;
                int tries = 0;
                do {
                    // Losuj pozycjÍ na ca≥ej mapie
                    float rx = (float)(rand() % (int)mapWidth);
                    float ry = (float)(rand() % (int)windowHeight * 2); // TrochÍ szerzej niø ekran
                    // Jeúli chcesz spawnowaÊ poza ekranem, najlepiej losowaÊ wokÛ≥ gracza, ale tutaj
                    // dla uproszczenia losujemy na mapie i sprawdzamy dystans.

                    spawnPos = sf::Vector2f(rx, ry);

                    // Oblicz dystans
                    sf::Vector2f diff = spawnPos - player.getPosition();
                    distToPlayer = std::sqrt(diff.x * diff.x + diff.y * diff.y);

                    tries++;
                } while (distToPlayer < 600.f && tries < 10); // Minimum 600px od gracza

                e.shape.setPosition(spawnPos);
                enemies.push_back(e);

                spawnClock.restart();
            }

            // 3. BRONIE (STRZELANIE)
            for (auto& w : weaponInventory) {
                w.cooldownTimer -= dt;

                // Czosnek jako strefa
                if (w.def.id == GARLIC) {
                    bool zoneExists = false;
                    for (auto& z : zones) if (z.wId == GARLIC) zoneExists = true;
                    if (!zoneExists) {
                        DamageZone z; z.wId = GARLIC; z.maxLifeTime = 99999.f;
                        z.damage = w.def.damage * pStats.might; // Apply might
                        // Apply Area
                        float r = 100.f * pStats.area;
                        z.shape.setRadius(r); z.shape.setFillColor(sf::Color(255, 200, 200, 100)); z.shape.setOrigin({ r, r });
                        zones.push_back(z);
                    }
                    continue;
                }

                if (w.cooldownTimer <= 0.f) {
                    w.cooldownTimer = w.def.baseCooldown * pStats.cooldown; // Apply Cooldown reduction

                    // Logika Amount (Duplicator)
                    int amount = 1 + pStats.amount;
                    if (w.def.id == WHIP || w.def.id == BIBLE || w.def.id == GARLIC) amount = 1; // Te bronie nie zawsze siÍ duplikujπ w prosty sposÛb
                    if (w.def.id == FIRE_WAND) amount += 2; // Baza ma 3

                    for (int i = 0; i < amount; ++i) {
                        Projectile p;
                        p.wId = w.def.id;
                        p.damage = w.def.damage * pStats.might; // Apply Might
                        float sizeMult = pStats.area;
                        p.maxLifeTime = w.def.duration * pStats.duration; // Apply Duration
                        float speed = w.def.speed * pStats.speed; // Apply Speed

                        // Inicjalizacja kszta≥tu
                        if (w.def.id == WHIP) {
                            p.boxShape.setSize({ 150.f * sizeMult, 40.f * sizeMult });
                            p.boxShape.setFillColor(w.def.color); p.boxShape.setOrigin({ 0.f, 20.f * sizeMult });
                            p.startPos = player.getPosition(); p.boxShape.setPosition(player.getPosition());
                            // Kolejne bicze bijπ w drugπ stronÍ (prosta logika)
                            if (i % 2 == 0) { if (facingDir == -1) p.boxShape.setScale({ -1.f, 1.f }); }
                            else { if (facingDir == 1) p.boxShape.setScale({ -1.f, 1.f }); } // BiÊ w plecy
                            p.pierceLeft = 999;
                        }
                        else {
                            // Circle shapes
                            float rad = 10.f * sizeMult;
                            if (w.def.id == AXE) rad = 15.f * sizeMult;
                            p.shape.setRadius(rad); p.shape.setFillColor(w.def.color); p.shape.setOrigin({ rad, rad });
                            p.shape.setPosition(player.getPosition());

                            if (w.def.id == MAGIC_WAND) {
                                float minDist = 10000.f; sf::Vector2f target = player.getPosition() + sf::Vector2f(100, 0);
                                for (auto& e : enemies) { float d = vectorLength(e.shape.getPosition() - player.getPosition()); if (d < minDist) { minDist = d; target = e.shape.getPosition(); } }
                                // Rozrzut dla wielu pociskÛw
                                sf::Vector2f dir = normalize(target - player.getPosition());
                                if (amount > 1) {
                                    float angle = (i - amount / 2.0f) * 0.2f;
                                    float ca = cos(angle), sa = sin(angle);
                                    dir = { dir.x * ca - dir.y * sa, dir.x * sa + dir.y * ca };
                                }
                                p.velocity = dir * speed;
                                p.pierceLeft = 0;
                            }
                            else if (w.def.id == KNIFE) {
                                sf::Vector2f dir((float)facingDir, 0.f);
                                if (amount > 1) {
                                    float angle = (i - amount / 2.0f) * 0.1f;
                                    dir = { dir.x, dir.y + angle }; // Lekki rozrzut pionowy
                                }
                                p.velocity = normalize(dir) * speed;
                                p.pierceLeft = 1 + (w.level / 2);
                            }
                            else if (w.def.id == AXE) {
                                float xDir = (float)facingDir * 300.f + (i * 50.f * facingDir);
                                p.velocity = sf::Vector2f(xDir, -700.f - (i * 50.f));
                                p.pierceLeft = 999;
                            }
                            else if (w.def.id == BOOMERANG) {
                                p.startPos = player.getPosition();
                                sf::Vector2f target = player.getPosition() + sf::Vector2f(100, 0);
                                if (!enemies.empty()) target = enemies[rand() % enemies.size()].shape.getPosition();
                                p.velocity = normalize(target - player.getPosition()) * speed;
                                p.pierceLeft = 999;
                            }
                            else if (w.def.id == BIBLE) {
                                p.pierceLeft = 999; // Obs≥uøone w logice ruchu
                            }
                            else if (w.def.id == FIRE_WAND) {
                                float angle = (float)(rand() % 360) * 3.14f / 180.f;
                                p.velocity = sf::Vector2f(std::cos(angle), std::sin(angle)) * speed;
                                p.pierceLeft = 999;
                            }
                            else if (w.def.id == HOLY_WATER) {
                                float angle = (float)(rand() % 360);
                                p.velocity = sf::Vector2f(std::cos(angle), std::sin(angle)) * speed;
                                p.maxLifeTime = 0.5f; p.pierceLeft = 0;
                            }
                            else if (w.def.id == RUNETRACER) {
                                p.isRunetracer = true; float angle = (float)(rand() % 360);
                                p.velocity = sf::Vector2f(std::cos(angle), std::sin(angle)) * speed;
                                p.pierceLeft = 999;
                            }
                            else if (w.def.id == LIGHTNING) {
                                if (!enemies.empty()) {
                                    int idx = rand() % enemies.size();
                                    enemies[idx].shape.setPosition({ -9999, -9999 });
                                    p.boxShape.setSize({ 20.f * sizeMult, (float)windowHeight });
                                    p.boxShape.setFillColor(w.def.color); p.boxShape.setOrigin({ 10.f * sizeMult, (float)windowHeight });
                                    p.boxShape.setPosition(enemies[idx].shape.getPosition()); p.maxLifeTime = 0.15f;

                                    ExpOrb orb; orb.shape.setRadius(5.f); orb.shape.setFillColor(sf::Color::Cyan); orb.shape.setOrigin({ 5.f,5.f });
                                    orb.shape.setPosition(enemies[idx].shape.getPosition()); orb.velocity = { 0,0 }; orb.value = 10;
                                    expOrbs.push_back(orb);
                                }
                            }
                        }
                        projectiles.push_back(p);
                    }
                }
            }

            // 4. AKTUALIZACJA POCISK”W
            for (size_t i = 0; i < projectiles.size();) {
                Projectile& p = projectiles[i];
                p.lifeTime += dt;
                bool dead = false;

                if (p.lifeTime > p.maxLifeTime && p.wId != BIBLE) {
                    if (p.wId == HOLY_WATER) {
                        DamageZone z; z.wId = HOLY_WATER; z.maxLifeTime = 3.0f * pStats.duration;
                        z.damage = p.damage;
                        float r = 60.f * pStats.area;
                        z.shape.setRadius(r); z.shape.setFillColor(sf::Color(0, 0, 255, 100)); z.shape.setOrigin({ r, r });
                        z.shape.setPosition(p.shape.getPosition());
                        zones.push_back(z);
                    }
                    dead = true;
                }

                if (!dead) {
                    // Logika ruchu (bicz, axe, bible...)
                    if (p.wId == AXE) {
                        p.velocity.y += 1000.f * dt;
                        p.shape.move(p.velocity * dt);
                        p.shape.rotate(sf::degrees(360.f * dt));
                    }
                    else if (p.wId == BOOMERANG) {
                        p.shape.move(p.velocity * dt);
                        p.shape.rotate(sf::degrees(720.f * dt));
                        if (!p.returning && p.lifeTime > p.maxLifeTime * 0.4f) { p.velocity = -p.velocity; p.returning = true; }
                    }
                    else if (p.wId == BIBLE) {
                        float speed = 3.0f * pStats.speed;
                        float dist = 120.f * pStats.area;
                        float angle = gameTime * speed + (p.lifeTime * 2.0f);
                        p.shape.setPosition(player.getPosition() + sf::Vector2f(std::cos(angle) * dist, std::sin(angle) * dist));
                    }
                    else if (p.wId == WHIP) { p.boxShape.setPosition(player.getPosition()); }
                    else if (p.wId == LIGHTNING) {}
                    else { p.shape.move(p.velocity * dt); }

                    // Kolizje
                    sf::FloatRect pBounds = (p.wId == WHIP || p.wId == LIGHTNING) ? p.boxShape.getGlobalBounds() : p.shape.getGlobalBounds();

                    for (auto& en : enemies) {
                        bool hitAlready = false; for (int id : p.hitEnemies) if (id == en.id) hitAlready = true;
                        if (hitAlready) continue;

                        // KOLIZJA POCISKU Z WROGIEM
                        if (pBounds.findIntersection(en.shape.getGlobalBounds())) {
                            p.hitEnemies.push_back(en.id);

                            // 1. ZADAJ OBRAØENIA
                            en.hp -= p.damage;

                            // Efekt trafienia (opcjonalny - migniÍcie na bia≥o)
                            // en.shape.setFillColor(sf::Color::White); 

                            // 2. SPRAWDè CZY WR”G ØYJE
                            if (en.hp <= 0.f) {
                                // åMIER∆ WROGA
                                sf::Vector2f deathPos = en.shape.getPosition();
                                en.shape.setPosition(sf::Vector2f(-9000.f, -9000.f)); // WyrzuÊ poza mapÍ (do usuniÍcia)

                                // Drop XP
                                ExpOrb orb;
                                orb.shape.setRadius(5.f);
                                orb.shape.setFillColor(sf::Color::Cyan);
                                orb.shape.setOrigin(sf::Vector2f(5.f, 5.f));
                                orb.shape.setPosition(deathPos);
                                float randX = (float)(rand() % 200 - 100);
                                orb.velocity = { randX, -400.f };

                                // WiÍcej XP za silniejszych wrogÛw
                                if (en.maxHp >= 10.f) orb.value = 50;
                                else if (en.maxHp >= 5.f) orb.value = 20;
                                else orb.value = 10;

                                expOrbs.push_back(orb);

                                // Waøne: Øeby usunπÊ wroga z wektora enemies, musimy to zrobiÊ ostroønie.
                                // Tutaj oznaczamy go jako "martwego" przesuwajπc daleko, 
                                // a pÍtla wrogÛw (niøej w kodzie) usunie go, bo x < -10000.
                                // To najbezpieczniejsza metoda bez przepisywania ca≥ej pÍtli pociskÛw.
                            }
                            else {
                                // PRZEØY£ - Odrzut (opcjonalne)
                                en.shape.move(p.velocity * dt * 0.5f);
                            }

                            // Obs≥uga Runetracer i Przebicia (bez zmian)
                            if (p.isRunetracer) {
                                float minDist = 10000.f; sf::Vector2f nextT = player.getPosition();
                                for (auto& other : enemies) {
                                    if (other.id == en.id) continue;
                                    float d = vectorLength(other.shape.getPosition() - p.shape.getPosition());
                                    if (d < minDist) { minDist = d; nextT = other.shape.getPosition(); }
                                }
                                p.velocity = normalize(nextT - p.shape.getPosition()) * 800.f;
                            }
                            if (p.pierceLeft > 0) p.pierceLeft--; else { dead = true; break; }
                        }
                    }
                }
                if (dead) projectiles.erase(projectiles.begin() + i); else i++;
            }

            // 5. STREFY OBRAØE—
            for (size_t i = 0; i < zones.size();) {
                DamageZone& z = zones[i];
                z.lifeTime += dt; z.tickTimer += dt;

                if (z.wId == GARLIC) z.shape.setPosition(player.getPosition());

                if (z.tickTimer > 0.2f) {
                    z.tickTimer = 0.f;
                    for (auto& en : enemies) {
                        if (z.shape.getGlobalBounds().findIntersection(en.shape.getGlobalBounds())) {

                            sf::Vector2f deathPos = en.shape.getPosition(); // Zapisz pozycjÍ
                            en.shape.setPosition({ -9000, -9000 });         // UsuÒ wroga

                            ExpOrb orb;
                            orb.shape.setRadius(5.f);
                            orb.shape.setFillColor(sf::Color::Cyan);
                            orb.shape.setOrigin({ 5.f,5.f });

                            orb.shape.setPosition(deathPos); // Uøyj deathPos

                            orb.velocity = { 0,0 };
                            orb.value = 10;
                            expOrbs.push_back(orb);
                        }
                    }
                }
                if (z.lifeTime > z.maxLifeTime) zones.erase(zones.begin() + i); else i++;
            }

            // 6. RUCH WROG”W
            for (size_t i = 0; i < enemies.size();) {
                if (enemies[i].shape.getPosition().x < -10000) { enemies.erase(enemies.begin() + i); continue; }

                // Celujemy w hitbox
                sf::Vector2f dir = normalize(hitbox.getPosition() - enemies[i].shape.getPosition());

                if (hitbox.getPosition().x < enemies[i].shape.getPosition().x) enemies[i].velocity.x = -150.f;
                else enemies[i].velocity.x = 150.f;

                enemies[i].velocity.y += baseGravity * dt;
                enemies[i].shape.move(enemies[i].velocity * dt);

                // ... (kod pod≥ogi wrogÛw bez zmian) ...
                if (enemies[i].shape.getPosition().y + 25.f > ground.getPosition().y) {
                    enemies[i].shape.setPosition({ enemies[i].shape.getPosition().x, ground.getPosition().y - 25.f });
                    enemies[i].velocity.y = 0.f;
                }

                // ZMIANA: Obs≥uga obraøeÒ i úmierci
                if (enemies[i].shape.getGlobalBounds().findIntersection(hitbox.getGlobalBounds()) && !isDying) {
                    float dmg = 1.0f;
                    if (pStats.armor > 0) dmg -= (float)pStats.armor * 0.1f;
                    if (dmg < 0.1f) dmg = 0.1f;

                    currentHp -= dmg;
                    enemies.erase(enemies.begin() + i);

                    if (currentHp <= 0) {
                        // Zaczynamy umieranie (animacja)
                        isDying = true;
                        isHurt = false; // åmierÊ nadpisuje obraøenia
                        bgMusic.stop();        // Zatrzymaj muzykÍ w tle
                        gameOverSound.play();
                    }
                    else {
                        // Otrzymanie obraøeÒ (przeøy≥)
                        isHurt = true;
                    }
                }
                else {
                    i++;
                }
            }

            // 7. EXP ORBS
            for (size_t i = 0; i < expOrbs.size();) {
                // Magnes
                float magnetRange = pStats.magnet;
                sf::Vector2f dirToP = hitbox.getPosition() - expOrbs[i].shape.getPosition();
                if (vectorLength(dirToP) < magnetRange) {
                    expOrbs[i].velocity = normalize(dirToP) * 1200.f;
                    expOrbs[i].isOnGround = false;
                }
                else if (!expOrbs[i].isOnGround) {
                    expOrbs[i].velocity.y += 1500.f * dt;
                }

                expOrbs[i].shape.move(expOrbs[i].velocity * dt);

                if (expOrbs[i].shape.getPosition().y + 5.f > ground.getPosition().y) {
                    expOrbs[i].shape.setPosition({ expOrbs[i].shape.getPosition().x, ground.getPosition().y - 5.f });
                    expOrbs[i].velocity.y = 0.f; expOrbs[i].velocity.x = 0.f; expOrbs[i].isOnGround = true;
                }

                if (expOrbs[i].shape.getGlobalBounds().findIntersection(hitbox.getGlobalBounds())) {
                    exp += expOrbs[i].value;
                    if (exp >= nextExp) {
                        exp -= nextExp;
                        nextExp = (int)(nextExp * 1.2f);
                        level++;

                        // GENEROWANIE KART LEVEL UP
                        upgradeCards.clear();

                        // Pula dostÍpnych opcji
                        std::vector<std::pair<int, int>> pool; // {type, index}

                        if (weaponInventory.size() < 3) {
                            for (int k = 0; k < (int)weaponDB.size(); ++k) {
                                bool has = false; for (auto& w : weaponInventory) if (w.def.id == weaponDB[k].id) has = true;
                                if (!has) pool.push_back({ 0, k });
                            }
                        }
                        for (auto& w : weaponInventory) {
                            if (w.level < 7) {
                                for (int k = 0; k < (int)weaponDB.size(); ++k) if (weaponDB[k].id == w.def.id) pool.push_back({ 0, k });
                            }
                        }

                        if (passiveInventory.size() < 3) {
                            for (int k = 0; k < (int)passiveDB.size(); ++k) {
                                bool has = false; for (auto& p : passiveInventory) if (p.def.id == passiveDB[k].id) has = true;
                                if (!has) pool.push_back({ 1, k });
                            }
                        }
                        for (auto& p : passiveInventory) {
                            if (p.level < p.def.maxLevel) {
                                for (int k = 0; k < (int)passiveDB.size(); ++k) if (passiveDB[k].id == p.def.id) pool.push_back({ 1, k });
                            }
                        }

                        std::shuffle(pool.begin(), pool.end(), std::default_random_engine(std::time(0)));
                        int count = std::min((int)pool.size(), 3);

                        float cardW = 300.f, gap = 50.f;
                        float startX = (windowWidth - (count * cardW + (count - 1) * gap)) / 2.f;

                        for (int c = 0; c < count; ++c) {
                            upgradeCards.emplace_back(font);
                            UpgradeCard& card = upgradeCards.back();

                            int type = pool[c].first;
                            int idx = pool[c].second;
                            card.type = type;
                            card.index = idx;

                            card.shape.setSize({ cardW, 400.f });
                            card.shape.setFillColor(sf::Color(40, 40, 40));
                            card.shape.setOutlineThickness(2.f);
                            card.shape.setPosition({ startX + c * (cardW + gap), 300.f });

                            std::string titleStr, descStr;
                            sf::Color col;
                            bool isUp = false;
                            int nextLvl = 1;

                            if (type == 0) { // BroÒ
                                titleStr = weaponDB[idx].name;
                                descStr = weaponDB[idx].description;
                                col = weaponDB[idx].color;
                                for (auto& w : weaponInventory) if (w.def.id == weaponDB[idx].id) { isUp = true; nextLvl = w.level + 1; }
                            }
                            else { // Pasywka
                                titleStr = passiveDB[idx].name;
                                descStr = passiveDB[idx].description;
                                col = passiveDB[idx].color;
                                for (auto& p : passiveInventory) if (p.def.id == passiveDB[idx].id) { isUp = true; nextLvl = p.level + 1; }
                            }

                            card.shape.setOutlineColor(col);
                            card.mainText.setString(titleStr);
                            card.mainText.setCharacterSize(28);
                            card.mainText.setFillColor(col);
                            centerText(card.mainText, card.shape.getPosition().x + 150.f, card.shape.getPosition().y + 50.f);

                            std::string sub = isUp ? "Ulepsz do poz. " + std::to_string(nextLvl) : "Nowy Przedmiot!";
                            card.subText.setString(sub + "\n\n" + descStr);
                            card.subText.setCharacterSize(20);
                            card.subText.setFillColor(sf::Color::White);

                            sf::FloatRect b = card.subText.getLocalBounds();
                            card.subText.setOrigin({ b.size.x / 2.f, 0.f });
                            card.subText.setPosition({ card.shape.getPosition().x + 150.f, card.shape.getPosition().y + 100.f });
                        }

                        if (!upgradeCards.empty()) isLevelUp = true;
                    }
                    expOrbs.erase(expOrbs.begin() + i);
                }
                else {
                    i++;
                }
            }
        }

        // --- RYSOWANIE ---
        window.clear(sf::Color(20, 20, 20));

        if (gameStarted) {
            sf::View view(sf::FloatRect({ 0.f, 0.f }, { (float)windowWidth, (float)windowHeight }));
            float camX = std::clamp(player.getPosition().x, windowWidth / 2.f, mapWidth - windowWidth / 2.f);
            view.setCenter({ camX, windowHeight / 2.f });
            window.setView(view);

            window.draw(background);
            window.draw(ground);

            for (auto& z : zones) window.draw(z.shape);
            for (auto& o : expOrbs) window.draw(o.shape);
            for (auto& e : enemies) window.draw(e.shape);
            for (auto& p : projectiles) {
                if (p.wId == WHIP || p.wId == LIGHTNING) window.draw(p.boxShape);
                else window.draw(p.shape);
            }
            window.draw(player);

            window.setView(window.getDefaultView());
            window.draw(uiBar);

            // RYSOWANIE PASKA BOSSA
            for (const auto& en : enemies) {
                if (en.isBoss) {
                    // Oblicz procent øycia
                    float hpPercent = en.hp / en.maxHp;
                    if (hpPercent < 0.f) hpPercent = 0.f;

                    // Aktualizuj szerokoúÊ paska
                    // Pasek ma max 800 szerokoúci. Musimy skalowaÊ wzglÍdem úrodka.
                    bossBarFill.setSize(sf::Vector2f(800.f * hpPercent, 30.f));

                    // Poniewaø setOrigin jest na úrodku, skalowanie dzia≥a symetrycznie, 
                    // ale SFML skaluje od origin. Øeby pasek mala≥ do lewej, musimy zmieniÊ logikÍ origin.
                    // UproúÊmy: narysujemy t≥o, a potem pasek øycia na nim.

                    // Rysuj
                    window.draw(bossBarBg);
                    window.draw(bossBarFill);
                    window.draw(bossNameText);
                    break; // Mamy tylko jednego bossa, nie szukaj dalej
                }
            }

            hpText.setString("HP: " + std::to_string((int)currentHp) + "/" + std::to_string(pStats.maxHp));
            window.draw(hpText);

            lvlText.setString("LVL: " + std::to_string(level) + " (" + std::to_string(exp) + "/" + std::to_string(nextExp) + ")");
            window.draw(lvlText);

            int m = (int)gameTime / 60; int s = (int)gameTime % 60;
            timeText.setString((m < 10 ? "0" : "") + std::to_string(m) + ":" + (s < 10 ? "0" : "") + std::to_string(s));
            centerText(timeText, windowWidth / 2.f, 50.f);
            window.draw(timeText);

            std::string wList = "Bronie:\n";
            for (auto& w : weaponInventory) wList += w.def.name + " " + std::to_string(w.level) + "\n";
            weaponsListText.setString(wList);
            window.draw(weaponsListText);

            std::string pList = "Pasywki:\n";
            for (auto& p : passiveInventory) pList += p.def.name + " " + std::to_string(p.level) + "\n";
            passivesListText.setString(pList);
            window.draw(passivesListText);

            if (isLevelUp) {
                window.draw(overlay);
                sf::Text upTxt(font); upTxt.setString("LEVEL UP!"); upTxt.setCharacterSize(60); upTxt.setFillColor(sf::Color::Yellow);
                centerText(upTxt, windowWidth / 2.f, 150.f);
                window.draw(upTxt);

                for (auto& c : upgradeCards) {
                    window.draw(c.shape);
                    window.draw(c.mainText);
                    window.draw(c.subText);
                }
            }
            else if (isGameOver) {
                window.draw(overlay);
                window.draw(gameOverTitle);
                window.draw(restartButton);
            }
            else if (isPaused) {
                window.draw(overlay);
                window.draw(pauseTitle);
                window.draw(resumeInfo);
                window.draw(pauseRestart);
            }
        }

        window.display();
    }

    return 0;
}