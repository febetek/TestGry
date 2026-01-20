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
	BIBLE, FIRE_WAND, GARLIC, HOLY_WATER, LIGHTNING
};

enum PassiveId {
	HOLLOW_HEART,   // 1. Max HP
	EMPTY_TOME,     // 2. Szybkostrzelnosc
	BRACER,         // 3. Szybkosc pocisku
	CANDLE,         // 4. Obszar
	SPELLBINDER,    // 5. Czas trwania
	SPINACH,        // 6. Obrazenia
	PUMMAROLA,      // 7. Regeneracja
	ATTRACTORB,     // 8. Magnes
	ARMOR,          // 9. Pancerz
	DUPLICATOR      // 10. Ilosc pociskow (Max lvl 3)
};

// --- STRUKTURY DANYCH ---

struct WeaponLevelStats {
	float damage;
	float cooldown;
	float duration;
	float speed;
	float area;
	int amount;        // Ile pociskÛw bazowo
	int pierce;
	std::string description; // Opis ulepszenia, np. "+1 Damage"
};

struct WeaponDef {
	WeaponId id;
	std::string name;
	sf::Color color;

	// Lista poziomÛw. levels[0] to lvl 1, levels[1] to lvl 2 itd.
	std::vector<WeaponLevelStats> levels;
};

struct ActiveWeapon {
	WeaponDef* def; // Wskaünik do bazy danych zeby nie kopiowaÊ ca≥ej listy poziomÛw
	int level = 1;
	float cooldownTimer = 0.0f;
};

struct PassiveDef {
	PassiveId id;
	std::string name;
	std::string description;
	sf::Color color;
	int maxLevel; // 7 dla wiÍkszoúci, 3 dla duplikatora
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
	float damage = 1.0f; // Obraøenia pocisku
	float angleOffset = 0.0f;
	float baseSpeed = 0.0f;
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
	sf::Sprite sprite;
	sf::Vector2f velocity;
	int value = 0;
	bool isOnGround = false;
	float animTimer = 0.0f;
	int currentFrame = 0;

	bool isMagnet = false;

	ExpOrb(const sf::Texture& tex) : sprite(tex) {
		value = 0;
		isOnGround = false;
		animTimer = 0.0f;
		currentFrame = 0;
		isMagnet = false;
		sprite.setScale(sf::Vector2f(1.5f, 1.5f));
	}
};

struct Portal {
	sf::Sprite sprite;
	float animTimer = 0.0f;
	int currentFrame = 0;

	Portal(const sf::Texture& tex, float x, float y) : sprite(tex) {
		sprite.setTextureRect(sf::IntRect({ 0, 0 }, { 32, 32 }));
		sprite.setOrigin({ 20.f, 30.f });
		sprite.setPosition({ x, y });
		sprite.setScale({ 5.0f, 5.0f });
	}
};

struct Lightning {

	sf::Sprite sprite;
	float animTimer = 0.0f;
	int currentFrame = 0;
	float lifeTime = 0.2f; // ile istnieje (sekundy)


	Lightning(const sf::Texture& tex, sf::Vector2f pos) : sprite(tex) {
		sprite.setTextureRect({ {0, 0}, {64, 64} });
		sprite.setOrigin({ 16.f, 56.f });
		sprite.setPosition(pos);
		sprite.setScale({ 4.f, 4.f });

	}
};

struct OchronaAnim {

	sf::Sprite sprite;
	float animTimer = 0.0f;
	int currentFrame = 0;
	float lifeTime = 2.0f; // ile animacja istnieje, np. 1 sekunda
	int targetId = -1;

	OchronaAnim(const sf::Texture& tex, sf::Vector2f pos) : sprite(tex) {
		sprite.setTextureRect({ {0, 0}, {128, 128} });
		sprite.setOrigin({ 63.f, 113.f });
		sprite.setPosition(pos);
		sprite.setScale({ 2.3f, 2.3f });
	}
};

struct SzczurAnim {

	sf::Sprite sprite;
	float animTimer = 0.0f;
	int currentFrame = 0;
	float lifeTime = 2.0f; // ile animacja istnieje, np. 1 sekunda

	int targetId = -1;

	SzczurAnim(const sf::Texture& tex, sf::Vector2f pos) : sprite(tex) {
		sprite.setTextureRect({ {0, 0}, {48, 48} });
		sprite.setOrigin({ 25.f, 19.f });
		sprite.setPosition(pos);
		sprite.setScale({ 2.0f, 2.0f });
	}
};
struct EgzaminAnim {
	sf::Sprite sprite;
	float animTimer = 0.0f;
	int currentFrame = 0;
	int targetId = -1; // ID wroga, do ktÛrego przypisana jest animacja

	EgzaminAnim(const sf::Texture& tex, sf::Vector2f pos) : sprite(tex) {
		// Ustawiamy wycinek tekstury na pierwszπ klatkÍ (38x38)
		sprite.setTextureRect({ {0, 0}, {38, 38} });

		// Ustawiamy úrodek (origin) na po≥owie wymiarÛw (19, 19)
		sprite.setOrigin({ 19.f, 19.f });

		sprite.setPosition(pos);
		// Skala 2.5f, øeby by≥ trochÍ wiÍkszy od szczura (moøesz zmieniÊ)
		sprite.setScale({ 2.0f, 2.0f });
	}
};

struct BossAnim {
	sf::Sprite sprite;
	float animTimer = 0.0f;
	int currentFrame = 0;
	int targetId = -1;

	BossAnim(const sf::Texture& tex, sf::Vector2f pos) : sprite(tex) {
		// Klatka 64x64
		sprite.setTextureRect({ {0, 0}, {64, 64} });

		// årodek w po≥owie (32, 32)
		sprite.setOrigin({ 32.f, 32.f });

		sprite.setPosition(pos);

		// Skala 4.0, øeby by≥ duøy (boss ma duøy hitbox radius=60)
		sprite.setScale({ 4.0f, 4.0f });
	}
};

struct Scarf {
	sf::Sprite sprite;
	float animTimer = 0.0f;
	int currentFrame = 0;
	bool finished = false;
	int side = 1;
	float scaleMult = 1.0f; // NOWE: Mnoønik wielkoúci

	// Konstruktor przyjmuje teraz 'sm' (sizeMult)
	Scarf(const sf::Texture& tex, sf::Vector2f pos, int dir, int s, float sm)
		: sprite(tex), side(s), scaleMult(sm)
	{
		sprite.setTextureRect(sf::IntRect({ 0, 0 }, { 1024, 1024 }));
		sprite.setOrigin({ 512.f, 512.f });

		// Obliczamy pozycjÍ (offset teø skalujemy, øeby szalik nie "wchodzi≥" w gracza przy duøym rozmiarze)
		float offsetDir = (float)(dir * side);
		sprite.setPosition(pos + sf::Vector2f(65.f * offsetDir, -5.f));

		// Ustawiamy startowπ skalÍ uwzglÍdniajπc mnoønik (bazowe 0.125f * mnoønik)
		float baseScale = 0.125f * scaleMult;

		if (offsetDir < 0) sprite.setScale({ -baseScale, baseScale });
		else sprite.setScale({ baseScale, baseScale });
	}
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

std::vector<WeaponDef> weaponDB;

void initWeaponDB() {
	WeaponDef whip;
	whip.id = WHIP;
	whip.name = "Szalik AGH";
	whip.color = sf::Color(139, 69, 19);
	// LVL 1 (Bazowy)
	//                      damage, cooldown, duration, speed, area, amount, pierce, discription
	whip.levels.push_back({ 10.f, 1.0f, 0.4f, 0.f, 1.0f, 1, 100, "Atakuje poziomo" });
	// LVL 2
	whip.levels.push_back({ 10.f, 1.0f, 0.4f, 0.f, 1.0f, 2, 100, "Ilosc +1 (Atakuje tyl)" });
	// LVL 3
	whip.levels.push_back({ 15.f, 1.0f, 0.4f, 0.f, 1.0f, 2, 100,  "Obrazenia +5" });
	// LVL 4
	whip.levels.push_back({ 20.f, 1.0f, 0.4f, 0.f, 1.1f, 2, 100, "Obrazenia +5 i Obszar 10%" });
	// LVL 5
	whip.levels.push_back({ 25.f, 1.0f, 0.4f, 0.f, 1.1f, 2, 100, "Obrazenia +5" });
	// LVL 6
	whip.levels.push_back({ 30.f, 1.0f, 0.4f, 0.f, 1.2f, 2, 100, "Obrazenia +5 i Obszar 10%" });
	// LVL 7
	whip.levels.push_back({ 35.f, 1.0f, 0.4f, 0.f, 1.2f, 2, 100, "Obrazenia +5" });

	weaponDB.push_back(whip);

	WeaponDef wand;
	wand.id = MAGIC_WAND;
	wand.name = "Piwo AGH-owskie";
	wand.color = sf::Color(0, 255, 255);

	wand.levels.push_back({ 5.f, 1.0f, 1.5f, 600.f, 1.0f, 1, 0, "Strzela w najblizszego wroga" });
	wand.levels.push_back({ 5.f, 1.0f, 1.5f, 600.f, 1.0f, 2, 0, "Ilosc +1" });
	wand.levels.push_back({ 5.f, 0.8f, 1.5f, 600.f, 1.0f, 2, 0, "Szybkostrzelnosc zwiekszona \no 20%" });
	wand.levels.push_back({ 5.f, 0.8f, 1.5f, 600.f, 1.0f, 3, 0, "Ilosc +1" });
	wand.levels.push_back({ 10.f, 0.8f, 1.5f, 600.f, 1.0f, 3, 0, "Obrazenia +5" });
	wand.levels.push_back({ 15.f, 0.8f, 1.5f, 600.f, 1.0f, 3, 0, "Obrazenia +5" });
	wand.levels.push_back({ 15.f, 0.8f, 1.5f, 600.f, 1.0f, 3, 1, "Przechodzi przez \njednego wroga wiecej" });
	weaponDB.push_back(wand);


	WeaponDef knife;
	knife.id = KNIFE;
	knife.name = "Olowek";
	knife.color = sf::Color(200, 200, 200);

	knife.levels.push_back({ 3.f, 1.0f, 1.0f, 900.f, 1.0f, 1, 1, "Leci w kierunku patrzenia" });
	knife.levels.push_back({ 3.f, 1.0f, 1.0f, 900.f, 1.0f, 2, 1, "Ilosc +1" });
	knife.levels.push_back({ 8.f, 1.0f, 1.0f, 900.f, 1.0f, 3, 1, "Ilosc +1 i obrazenia +5" });
	knife.levels.push_back({ 8.f, 1.0f, 1.0f, 900.f, 1.0f, 3, 1, "Przechodzi przez \njednego wroga wiecej" });
	knife.levels.push_back({ 8.f, 1.0f, 1.0f, 900.f, 1.0f, 4, 2, "Ilosc +1 przebicie +1" });
	knife.levels.push_back({ 13.f, 1.0f, 1.0f, 900.f, 1.0f, 4, 2, "Obrazenia +5" });
	knife.levels.push_back({ 13.f, 1.0f, 1.0f, 900.f, 1.0f, 4, 2, "Przechodzi przez \njednego wroga wiecej" });
	weaponDB.push_back(knife);

	WeaponDef axe;
	axe.id = AXE;
	axe.name = "Krzeslo";
	axe.color = sf::Color(129,69,19);

	axe.levels.push_back({ 20.f, 1.5f, 3.0f, 500.f, 1.0f, 1, 5, "Leci w gore po paraboli" });
	axe.levels.push_back({ 20.f, 1.5f, 3.0f, 500.f, 1.0f, 2, 5, "Ilosc +1" });
	axe.levels.push_back({ 40.f, 1.5f, 3.0f, 500.f, 1.0f, 2, 5, "Obrazenia +20" });
	axe.levels.push_back({ 40.f, 1.5f, 3.0f, 500.f, 1.0f, 3, 5, "Ilosc +1" });
	axe.levels.push_back({ 60.f, 1.5f, 3.0f, 500.f, 1.0f, 3, 5, "Obrazenia +20" });
	axe.levels.push_back({ 80.f, 1.5f, 3.0f, 500.f, 1.0f, 3, 10, "Obrazenia +20 przebicie +5" });
	axe.levels.push_back({ 80.f, 1.5f, 3.0f, 500.f, 1.5f, 3, 10, "Obszar zwiekszony o 50%" });
	weaponDB.push_back(axe);

	WeaponDef banan;
	banan.id = BOOMERANG;
	banan.name = "Banan";
	banan.color = sf::Color::Yellow;

	banan.levels.push_back({ 5.f, 1.0f, 2.0f, 600.f, 1.0f, 1, 1, "Leci i wraca jak bumerang" });
	banan.levels.push_back({ 10.f, 1.0f, 2.0f, 600.f, 1.0f, 1, 1, "Obrazenia +5" });
	banan.levels.push_back({ 10.f, 1.0f, 2.0f, 750.f, 1.1f, 1, 1, "Obszar zwiekszony o 10% \ni szybkosc o 25%" });
	banan.levels.push_back({ 10.f, 1.0f, 2.0f, 750.f, 1.1f, 2, 2, "Ilosc +1 przebicie +1" });
	banan.levels.push_back({ 15.f, 1.0f, 2.0f, 750.f, 1.1f, 2, 2, "Obrazenia +5" });
	banan.levels.push_back({ 15.f, 1.0f, 2.0f, 937.5f, 1.2f, 2, 2, "Obszar zwiekszony o 10% \ni szybkosc o 25%" });
	banan.levels.push_back({ 15.f, 1.0f, 2.0f, 937.5f, 1.2f, 3, 3, "Ilosc +1 przebicie +1" });
	weaponDB.push_back(banan);


	WeaponDef bible;
	bible.id = BIBLE;
	bible.name = "Podrecznik do analizy";
	bible.color = sf::Color(255, 0, 0);

	bible.levels.push_back({ 10.f, 1.0f, 3.0f, 600.f, 1.0f, 1, 100, "Orbituje wokol ciebie" });
	bible.levels.push_back({ 10.f, 1.0f, 3.0f, 600.f, 1.0f, 2, 100, "Ilosc +1" });
	bible.levels.push_back({ 10.f, 1.0f, 3.0f, 780.f, 1.25f, 2, 100, "Obszar zwiekszony o 25% \ni szybkosc o 30%" });
	bible.levels.push_back({ 15.f, 1.0f, 3.0f, 780.f, 1.25f, 2, 100, "Obrazenia +5" });
	bible.levels.push_back({ 15.f, 1.0f, 3.0f, 780.f, 1.25f, 3, 100, "Ilosc +1" });
	bible.levels.push_back({ 15.f, 1.0f, 3.0f, 1014.f, 1.50f, 3, 100, "Obszar zwiekszony o 25% \ni szybkosc o 30%" });
	bible.levels.push_back({ 20.f, 1.0f, 3.0f, 1014.f, 1.50f, 3, 100, "Obrazenia +5" });
	weaponDB.push_back(bible);


	WeaponDef metal;
	metal.id = FIRE_WAND;
	metal.name = "Kawalki stali";
	metal.color = sf::Color(128, 128, 128);

	metal.levels.push_back({ 20.f, 1.0f, 3.0f, 200.f, 1.0f, 3, 1, "Potezne pociski" });
	metal.levels.push_back({ 30.f, 1.0f, 3.0f, 200.f, 1.0f, 3, 3, "Obrazenia +10 przebicie +2" });
	metal.levels.push_back({ 40.f, 1.0f, 3.0f, 200.f, 1.0f, 4, 3, "Obrazenia +10 i ilosc +1" });
	metal.levels.push_back({ 50.f, 1.0f, 3.0f, 240.f, 1.0f, 4, 3, "Obrazenia +10 i szybkosc o 20%" });
	metal.levels.push_back({ 60.f, 1.0f, 3.0f, 240.f, 1.0f, 4, 5, "Obrazenia +10 przebicie +2" });
	metal.levels.push_back({ 70.f, 1.0f, 3.0f, 288.f, 1.0f, 4, 5, "Obrazenia +10 i szybkosc o 20%" });
	metal.levels.push_back({ 80.f, 1.0f, 3.0f, 288.f, 1.0f, 6, 5, "Obrazenia +10 i ilosc +2" });
	weaponDB.push_back(metal);


	WeaponDef stink;
	stink.id = GARLIC;
	stink.name = "Smierdzacy zapach";
	stink.color = sf::Color::Green;

	stink.levels.push_back({ 5.f, 1.0f, 3.0f, 600.f, 1.0f, 1, 1, "Aura raniaca wrogow" });
	stink.levels.push_back({ 7.f, 1.0f, 3.0f, 600.f, 1.25f, 1, 1, "Obszar zwiekszony o 25% \ni obrazenia +2" });
	stink.levels.push_back({ 9.f, 0.0f, 3.0f, 600.f, 1.25f, 1, 1, "Obrazenia +2" });
	stink.levels.push_back({ 9.f, 0.8f, 3.0f, 600.f, 1.25f, 1, 1, "Szybkostrzelnosc zwiekszona \no 20%" });
	stink.levels.push_back({ 11.f, 0.8f, 3.0f, 600.f, 1.50f, 1, 1, "Obszar zwiekszony o 25% \ni obrazenia +2" });
	stink.levels.push_back({ 11.f, 0.6f, 3.0f, 600.f, 1.50f, 1, 1,"Szybkostrzelnosc zwiekszona o 20%" });
	stink.levels.push_back({ 13.f, 0.6f, 3.0f, 600.f, 2.0f, 1, 1, "Obszar zwiekszony o 50% \ni obrazenia +2" });
	weaponDB.push_back(stink);


	WeaponDef flask;
	flask.id = HOLY_WATER;
	flask.name = "Fiolka z laboratorium";
	flask.color = sf::Color(0, 100, 255);

	flask.levels.push_back({ 15.f, 0.8f, 2.0f, 175.f, 1.0f, 1, 0, "Tworzy plame" });
	flask.levels.push_back({ 15.f, 0.8f, 2.0f, 175.f, 1.0f, 2, 0, "Ilosc +1" });
	flask.levels.push_back({ 25.f, 0.8f, 2.5f, 175.f, 1.0f, 2, 0, "Efekt trwa 0,5s dluzej \ni obrazenia +10" });                               //------------------------------
	flask.levels.push_back({ 25.f, 0.8f, 2.5f, 175.f, 1.0f, 3, 0, "Ilosc +1" });
	flask.levels.push_back({ 25.f, 0.8f, 2.8f, 175.f, 1.0f, 3, 0, "Efekt trwa 0,3s dluzej" });
	flask.levels.push_back({ 35.f, 0.8f, 2.8f, 175.f, 1.0f, 3, 0, "Obrazenia +10" });
	flask.levels.push_back({ 50.f, 0.8f, 3.3f, 175.f, 1.0f, 4, 0, "Efekt trwa 0,5s dluzej,\n ilosc +1 i obr +15" });
	weaponDB.push_back(flask);


	WeaponDef lightning;
	lightning.id = LIGHTNING;
	lightning.name = "Piorun";
	lightning.color = sf::Color::Yellow;
	lightning.levels.push_back({ 5.f, 1.2f, 0.2f, 0.f, 1.0f, 1, 100, "Razi losowych wrogow" });
	lightning.levels.push_back({ 7.f, 1.2f, 0.2f, 0.f, 1.0f, 1, 100, "Obrazenia +2" });
	lightning.levels.push_back({ 7.f, 1.2f, 0.2f, 0.f, 1.0f, 2, 100, "Ilosc +1" });
	lightning.levels.push_back({ 7.f, 1.0f, 0.2f, 0.f, 1.0f, 2, 100, "Szybkostrzelnosc zwiekszona \no 20%" });
	lightning.levels.push_back({ 8.f, 1.0f, 0.2f, 0.f, 1.0f, 2, 100, "Obrazenia +1" });
	lightning.levels.push_back({ 10.f, 1.0f, 0.2f, 0.f, 1.0f, 2, 100, "Obrazenia +2" });
	lightning.levels.push_back({ 10.f, 1.0f, 0.2f, 0.f, 1.0f, 3, 100, "Ilosc +1" });
	weaponDB.push_back(lightning);
}

std::vector<PassiveDef> passiveDB = {
	// ID, Nazwa, Opis, Kolor, Max lvl
	{HOLLOW_HEART, "Serce Witalnosci", "Zwieksza Max HP (+20%)", sf::Color(200, 0, 0), 5},
	{EMPTY_TOME, "Energetyk", "Szybkostrzelnosc (-8% cooldown)", sf::Color(200, 200, 0), 5},
	{BRACER, "Moc rzutu", "Szybkosc pocisku (+10%)", sf::Color(100, 100, 255), 5},
	{CANDLE, "Sterydy", "Obszar pocisku (+10%)", sf::Color(255, 150, 0), 5},
	{SPELLBINDER, "Warunek", "Czas trwania (+10%)", sf::Color(100, 0, 255), 5},
	{SPINACH, "Sila", "Obrazenia (+10%)", sf::Color(0, 150, 0), 5},
	{PUMMAROLA, "Odzywajacy kebs", "Regeneracja HP (+0.2/s)", sf::Color(255, 100, 100), 5},
	{ATTRACTORB, "Magnes", "Zasieg zbierania (+30%)", sf::Color(0, 0, 255), 5},
	{ARMOR, "Pancerz", "Redukcja obrazen (+1)", sf::Color(150, 150, 150), 5},
	{DUPLICATOR, "Duplikator chat GPT", "Ilosc pociskow (+1)", sf::Color(0, 255, 255), 2}
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
		case HOLLOW_HEART:
			stats.maxHp += (int)(bonus * 2);
			std::cout << "Max HP: " << stats.maxHp << std::endl;
			break;
		case EMPTY_TOME:
			stats.cooldown -= (bonus * 0.08f);
			std::cout << "Cooldown: " << stats.cooldown << std::endl;
			break;
		case BRACER:
			stats.speed += (bonus * 0.1f);
			std::cout << "Bullet speed: " << stats.speed << std::endl;
			break;
		case CANDLE:
			stats.area += (bonus * 0.1f);
			std::cout << "Area: " << stats.area << std::endl;
			break;
		case SPELLBINDER:
			stats.duration += (bonus * 0.1f);
			std::cout << "Duration: " << stats.duration << std::endl;
			break;
		case SPINACH:
			stats.might += (bonus * 0.1f);
			std::cout << "Damage: " << stats.might << std::endl;
			break;
		case PUMMAROLA:
			stats.regen += (bonus * 0.2f);
			std::cout << "Regen: " << stats.regen << " na sekunde" << std::endl;
			break;
		case ATTRACTORB:
			stats.magnet += (bonus * 50.f);
			std::cout << "Magnet: " << stats.magnet << std::endl;
			break;
		case ARMOR:
			stats.armor += (int)bonus;
			std::cout << "Armor: " << stats.armor << std::endl;
			break;
		case DUPLICATOR:
			stats.amount += (int)bonus;
			std::cout << "Duplicator: " << stats.amount + 1 << std::endl;
			break;
		}
	}
	// Limit cooldownu zeby nie bylo 0
	if (stats.cooldown < 0.1f) stats.cooldown = 0.1f;
	return stats;
}
// Funkcja do przypisywania klawiszy

void keyBindingsFunction(
	RenderWindow& window,
	Sound& clickSound,
	Text& buttonText,
	Keyboard::Scancode& keyVariable,
	const String& actionName,
	float targetX,
	float targetY,
	const Drawable& controlsTitle,
	bool& isFullscreen,
	unsigned int winWidth,
	unsigned int winHeight,
	Vector2f mousePos)
{
	if (buttonText.getGlobalBounds().contains(mousePos))
	{
		clickSound.play();
		Texture screenshotTexture;
		if (screenshotTexture.resize(window.getSize())) screenshotTexture.update(window);
		Sprite tlo(screenshotTexture);
		tlo.setColor(Color(50, 50, 50));
		bool inControls = true;
		while (window.isOpen() && inControls)
		{
			Vector2f mousePos = (Vector2f)Mouse::getPosition(window);

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
					// opcja usuniÍcia przypisania klawisza 
					else if (key->scancode == Keyboard::Scancode::Delete)
					{
						keyVariable = Keyboard::Scancode::Unknown;
						inControls = false;
					}
					// moøliwoúÊ zmienienia fullscreen na tryb okienkowy i odwrotnie za pomocπ klawisza F11
					else if (key->scancode == Keyboard::Scancode::F11)
					{
						isFullscreen = !isFullscreen;
						window.close();
						if (isFullscreen) window.create(VideoMode::getDesktopMode(), "AGH survival", Style::Default, State::Fullscreen);
						else window.create(VideoMode({ winWidth, winHeight }), "AGH survival", Style::Default, State::Windowed);
						window.setFramerateLimit(140);

					}
					// warunek, aby gra nie reagowa≥a na klikniÍcie okreúlonych klawiszy przy przypisywaniu klawiszy
					else if (
						key->scancode == Keyboard::Scancode::PrintScreen ||
						key->scancode == Keyboard::Scancode::ScrollLock ||
						key->scancode == Keyboard::Scancode::Pause ||
						key->scancode == Keyboard::Scancode::Insert ||
						key->scancode == Keyboard::Scancode::Home ||
						key->scancode == Keyboard::Scancode::PageUp ||
						key->scancode == Keyboard::Scancode::End ||
						key->scancode == Keyboard::Scancode::PageDown
						)
					{
					}
					// przypisywanie klawiszy
					else
					{
						keyVariable = key->scancode;
						inControls = false;
					}
				}
			}
			window.clear(Color(0x808080FF));
			window.draw(tlo);
			window.draw(controlsTitle);
			window.display();
		}
		String newKeyName;
		// zmiana nazwy klawisza po wciúniÍciu Delete
		if (keyVariable == Keyboard::Scancode::Unknown) {
			newKeyName = L"Unknown";
		}
		// zmiana nazwy klawisza po przypisaniu okreúlonego klawisza
		else {
			newKeyName = Keyboard::getDescription(keyVariable);
		}
		// zmiana nazwy klawisza i wycentrowanie tekstu 
		buttonText.setString(actionName + L" (" + newKeyName + L")");
		centerText(buttonText, targetX, targetY);
	}

}

int main() {
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	unsigned int windowWidth = 1920;
	unsigned int windowHeight = 1080;

	sf::RenderWindow window(sf::VideoMode({ windowWidth, windowHeight }), "AGH survival");
	window.setFramerateLimit(140);

	window.setKeyRepeatEnabled(false);//linia zeby przytrzymanie klawisza ESC nie robily sie dziwne rzeczy

	bool isFullscreen = false;

	sf::Image icon;
	if (icon.loadFromFile("sprites/icon.png")) {
		window.setIcon(icon.getSize(), icon.getPixelsPtr());
	}

	sf::Texture menuBgTexture;
	// PamiÍtaj o poprawnej úcieøce i nazwie pliku!
	if (!menuBgTexture.loadFromFile("sprites/mainmenu.png")) {
		std::cout << "Brak pliku menu_bg.png!" << std::endl;
		return -1;
	}
	sf::Sprite menuBgSprite(menuBgTexture);


	sf::Texture bgTexture;
	if (!bgTexture.loadFromFile("sprites/background.png")) {
		std::cout << "Brak pliku background.png!" << std::endl;
		return -1;
	}

	sf::Sprite background(bgTexture);
	float mapWidth = (float)bgTexture.getSize().x;

	initWeaponDB();//inicjalizacja bazy danych broni

	// wczytywanie czcionki
	sf::Font font;
	if (!font.openFromFile("czcionka/arial.ttf")) {
		std::cout << "Brak pliku arial.ttf!" << std::endl;
		return -1;
	}

	sf::Image cursorImage;
	std::optional<sf::Cursor> customCursor;

	if (cursorImage.loadFromFile("sprites/kursor.png")) {
		customCursor = sf::Cursor::createFromPixels(cursorImage.getPixelsPtr(),cursorImage.getSize(),{ 0, 0 });

		if (customCursor) { // Sprawdzamy czy uda≥o siÍ stworzyÊ kursor
			window.setMouseCursor(customCursor.value());
		}
	}

	// PORTAl
	sf::Texture portalTexture;
	if (!portalTexture.loadFromFile("animacje/portal.png")) {
		std::cout << "Brak pliku portal.png!" << std::endl;
		return -1;
	}
	std::vector<Portal> portals;
	float portalY = windowHeight - 85.f;

	portals.emplace_back(portalTexture, 100.f, portalY);// Lewy portal (na poczπtku mapy)
	portals.emplace_back(portalTexture, mapWidth - 100.f, portalY);// Prawy portal (na koÒcu mapy)
	// -------------

	sf::Texture expOrbTexture;
	if (!expOrbTexture.loadFromFile("animacje/exp_orb.png")) {
		std::cout << "Brak pliku exp_orb.png!" << std::endl;
		return -1;
	}

	sf::Texture piorunTexture;
	if (!piorunTexture.loadFromFile("animacje/piorun.png")) {
		std::cout << "Brak pliku piorun.png!" << std::endl;
		return -1;
	}
	std::vector<Lightning> lightnings;

	sf::Texture ochronaTexture;
	if (!ochronaTexture.loadFromFile("animacje/ochrona.png")) {
		std::cout << "Brak pliku ochrona.png!" << std::endl;
		return -1;
	}
	std::vector<OchronaAnim>Ochrona;

	sf::Texture szczurTexture;
	if (!szczurTexture.loadFromFile("animacje/szczur.png")) {
		std::cout << "Brak pliku szczur.png!" << std::endl;
		return -1;
	}
	std::vector<SzczurAnim>Szczur;

	sf::Texture egzaminTexture;
	if (!egzaminTexture.loadFromFile("animacje/egzamin.png")) { // Podaj tu swojπ nazwÍ pliku!
		std::cout << "Brak pliku enemy3.png!" << std::endl;
		return -1;
	}
	std::vector<EgzaminAnim> Egzamin;

	sf::Texture bossTexture;
	if (!bossTexture.loadFromFile("animacje/auto.png")) {
		std::cout << "Brak pliku boss.png!" << std::endl;
		return -1;
	}
	std::vector<BossAnim> Boss;

	sf::Texture szalikTexture;
	if (!szalikTexture.loadFromFile("animacje/szalik.png")) {
		std::cout << "Brak pliku szalik.png!" << std::endl;
		return-1;
	}
	std::vector<Scarf> szalik;


	sf::Texture olowekTexture;
	if (!olowekTexture.loadFromFile("sprites/olowek.png")) {
		std::cout << "Brak pliku olowek.png!" << std::endl;
		return-1;
	}

	sf::Texture krzesloTexture;
	if (!krzesloTexture.loadFromFile("sprites/krzeslo.png")) {
		std::cout << "Brak pliku krzeslo.png!" << std::endl;
		return-1;
	}

	sf::Texture ksiazkaTexture;
	if (!ksiazkaTexture.loadFromFile("sprites/ksiazka.png")) {
		std::cout << "Brak pliku ksiazka.png!" << std::endl;
		return-1;
	}

	sf::Texture piwoTexture;
	if (!piwoTexture.loadFromFile("sprites/piwo.png")) {
		std::cout << "Brak pliku piwo.png!" << std::endl;
		return-1;
	}

	sf::Texture potkaTexture;
	if (!potkaTexture.loadFromFile("sprites/potka.png")) {
		std::cout << "Brak pliku potka.png!" << std::endl;
		return-1;
	}

	sf::Texture stalTexture;
	if (!stalTexture.loadFromFile("sprites/stal.png")) {
		std::cout << "Brak pliku stal.png!" << std::endl;
		return-1;
	}

	sf::Texture bananTexture;
	if (!bananTexture.loadFromFile("sprites/banan.png")) {
		std::cout << "Brak pliku banan.png!" << std::endl;
		return-1;
	}

	sf::Texture magnesTexture;
	if (!magnesTexture.loadFromFile("sprites/magnes.png")) {
		std::cout << "Brak pliku magnes.png!" << std::endl;
		return -1;
	}

	//       --- AUDIO  ---
	sf::Music bgMusic;
	if (!bgMusic.openFromFile("dzwieki/music.ogg")) {
		std::cout << "Brak pliku music.ogg!" << std::endl;
		return -1;
	}
	float musicScale = 30.0f;
	bgMusic.setLooping(true); // <--- ZMIANA: setLooping zamiast setLoop                // ustawienia muzyki
	bgMusic.setVolume(musicScale);

	sf::SoundBuffer jumpBuffer;
	if (!jumpBuffer.loadFromFile("dzwieki/jump_sound.wav")) {
		std::cout << "Brak pliku jump_sound.wav!" << std::endl;
		return -1;
	}

	float sfxScale = 50.0f;

	sf::SoundBuffer expBuffer;
	if (!expBuffer.loadFromFile("dzwieki/exp.wav")) {
		std::cout << "Brak pliku exp.wav!" << std::endl;
		return -1;
	}
	sf::Sound expSound(expBuffer);
	expSound.setVolume(sfxScale * 0.3f);

	sf::Sound jumpSound(jumpBuffer);
	jumpSound.setVolume(sfxScale);                                                      // ustawienia g≥oúnoúci skoku

	sf::SoundBuffer clickBuffer;
	if (!clickBuffer.loadFromFile("dzwieki/click.wav")) {
		std::cout << "Brak pliku click.wav!" << std::endl;
		return -1;
	}
	sf::Sound clickSound(clickBuffer);
	clickSound.setVolume(sfxScale);                                                     // ustawienia g≥oúnoúci klikniÍcia

	sf::SoundBuffer gameOverBuffer;
	if (!gameOverBuffer.loadFromFile("dzwieki/game_over.ogg")) {
		std::cout << "Brak pliku game_over.ogg!" << std::endl;
		return -1;
	}
	sf::Sound gameOverSound(gameOverBuffer);                                                     // ustawienia düwiÍku gameover
	gameOverSound.setVolume(sfxScale * 1.5f);

	sf::SoundBuffer levelUpBuffer;
	if (!levelUpBuffer.loadFromFile("dzwieki/levelup.wav")) {
		std::cout << "Brak pliku levelup.wav!" << std::endl;
		return -1;
	}
	sf::Sound levelUpSound(levelUpBuffer);
	levelUpSound.setVolume(sfxScale * 1.2f);

	sf::SoundBuffer bossDeathBuffer;
	if (!bossDeathBuffer.loadFromFile("dzwieki/boss_dead.wav")) {
		std::cout << "Brak pliku boss_death.wav!" << std::endl;
		return -1;
	}
	sf::Sound bossDeathSound(bossDeathBuffer);
	bossDeathSound.setVolume(sfxScale * 2.f);

	sf::SoundBuffer bossHitBuffer;
	if (!bossHitBuffer.loadFromFile("dzwieki/boss_hit.wav")) {
		std::cout << "Brak pliku boss_hit.wav!" << std::endl;
		return -1;
	}
	sf::Sound bossHitSound(bossHitBuffer);
	bossHitSound.setVolume(sfxScale * 2.f);

	sf::SoundBuffer hurtBuffer;
	if (!hurtBuffer.loadFromFile("dzwieki/hurt.wav")) {
		std::cout << "Brak pliku hurt.wav!" << std::endl;
		return -1;
	}
	sf::Sound hurtSound(hurtBuffer);
	hurtSound.setVolume(sfxScale * 1.2f);

	sf::SoundBuffer lightningBuffer;
	if (!lightningBuffer.loadFromFile("dzwieki/piorun.wav")) {
		std::cout << "Brak pliku piorun.wav!" << std::endl;
		return -1;
	}
	sf::Sound lightningSound(lightningBuffer);
	lightningSound.setVolume(sfxScale * 0.8f);

	sf::SoundBuffer whipBuffer;
	if (!whipBuffer.loadFromFile("dzwieki/szalik.wav")) {
		std::cout << "Brak pliku szalik.wav!" << std::endl;
		return -1;
	}
	sf::Sound whipSound(whipBuffer);
	whipSound.setVolume(sfxScale * 0.8f);

	sf::SoundBuffer beerBuffer;
	if (!beerBuffer.loadFromFile("dzwieki/piwo.wav")) {
		std::cout << "Brak pliku piwo.wav!" << std::endl;
		return -1;
	}
	sf::Sound beerSound(beerBuffer);
	beerSound.setVolume(sfxScale * 0.6f);

	sf::SoundBuffer pencilBuffer;
	if (!pencilBuffer.loadFromFile("dzwieki/olowek.wav")) {
		std::cout << "Brak pliku olowek.wav!" << std::endl;
		return -1;
	}
	sf::Sound pencilSound(pencilBuffer);
	pencilSound.setVolume(sfxScale * 0.7f);

	sf::SoundBuffer chairBuffer;
	if (!chairBuffer.loadFromFile("dzwieki/krzeslo.wav")) {
		std::cout << "Brak pliku krzeslo.wav!" << std::endl;
		return -1;
	}
	sf::Sound chairSound(chairBuffer);
	chairSound.setVolume(sfxScale);

	sf::SoundBuffer bananaBuffer;
	if (!bananaBuffer.loadFromFile("dzwieki/banan.wav")) {
		std::cout << "Brak pliku banan.wav!" << std::endl;
		return -1;
	}
	sf::Sound bananaSound(bananaBuffer);
	bananaSound.setVolume(sfxScale * 0.8f);

	sf::SoundBuffer stalBuffer;
	if (!stalBuffer.loadFromFile("dzwieki/stal.wav")) {
		std::cout << "Brak pliku stal.wav!" << std::endl;
		return -1;
	}
	sf::Sound stalSound(stalBuffer);
	stalSound.setVolume(sfxScale * 0.8f);

	sf::SoundBuffer flaskBuffer;
	if (!flaskBuffer.loadFromFile("dzwieki/flask.wav")) {
		std::cout << "Brak pliku flask.wav!" << std::endl;
		return -1;
	}
	sf::Sound flaskSound(flaskBuffer);
	flaskSound.setVolume(sfxScale * 0.8f);

	sf::SoundBuffer bibleHitBuffer;
	if (!bibleHitBuffer.loadFromFile("dzwieki/bible_hit.wav")) {
		std::cout << "Brak pliku bible_hit.wav!" << std::endl;
		return -1;
	}
	sf::Sound bibleHitSound(bibleHitBuffer);
	bibleHitSound.setVolume(sfxScale * 0.8f);

	sf::SoundBuffer garlicHitBuffer;
	if (!garlicHitBuffer.loadFromFile("dzwieki/garlic_hit.wav")) {
		std::cout << "Brak pliku garlic_hit.wav!" << std::endl;
		return -1;
	}
	sf::Sound garlicHitSound(garlicHitBuffer);
	garlicHitSound.setVolume(sfxScale * 0.5f);

	// --- UI SETUP ---
	sf::RectangleShape uiBar({ (float)windowWidth, 100.f }); uiBar.setFillColor(sf::Color(0, 0, 0, 150));
	sf::RectangleShape overlay({ (float)windowWidth, (float)windowHeight }); overlay.setFillColor(sf::Color(0, 0, 0, 200));

	sf::Text hpText(font); hpText.setCharacterSize(30); hpText.setPosition({ 20.f, 10.f });
	sf::Text lvlText(font); lvlText.setCharacterSize(25); lvlText.setFillColor(sf::Color::Cyan); lvlText.setPosition({ 20.f, 50.f });
	sf::Text timeText(font); timeText.setCharacterSize(40);

	sf::Text weaponsListText(font); weaponsListText.setCharacterSize(18); weaponsListText.setPosition({ windowWidth - 400.f, 10.f });
	sf::Text passivesListText(font); passivesListText.setCharacterSize(18); passivesListText.setPosition({ windowWidth - 200.f, 10.f });

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

	sf::Text pauseMainMenu(font);
	pauseMainMenu.setString("MENU GLOWNE");
	pauseMainMenu.setCharacterSize(40);
	pauseMainMenu.setFillColor(sf::Color::White);
	centerText(pauseMainMenu, windowWidth / 2.f, windowHeight / 2.f + 190.f);

	sf::Text pauseStatsText(font);
	pauseStatsText.setCharacterSize(25);
	pauseStatsText.setFillColor(sf::Color::Cyan);
	pauseStatsText.setPosition({ windowWidth - 400.f, 200.f });

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
	sf::Text menuTitle(font);
	menuTitle.setString(" AGH Survival");
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
	ControlsTitle.setString(L"          WCIåNIJ KLAWISZ, KT”RY CHCESZ PRZYPISA∆,\n                  DELETE, ABY USUN•∆ PRZYPISANIE\n                                    LUB\n                           ESC, ABY WYJå∆");
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

	while (window.isOpen()) {

		// --- P TLA MENU G£”WNEGO ---
		while (window.isOpen() && inMainMenu) {
			sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window); // Update pozycji myszki na poczπtku klatki

			while (const std::optional event = window.pollEvent()) {
				if (event->is<sf::Event::Closed>()) { window.close(); return 0; }
				else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
					if (key->scancode == sf::Keyboard::Scancode::F11) {
						isFullscreen = !isFullscreen;
						window.close();
						if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "AGH survival", sf::Style::Default, sf::State::Fullscreen);
						else window.create(sf::VideoMode({ windowWidth, windowHeight }), "AGH survival", sf::Style::Default, sf::State::Windowed);
						window.setFramerateLimit(140);
						if (customCursor) {
							window.setMouseCursor(customCursor.value());
						}
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
																if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "AGH survival", sf::Style::Default, sf::State::Fullscreen);
																else window.create(sf::VideoMode({ windowWidth, windowHeight }), "AGH survival", sf::Style::Default, sf::State::Windowed);
																window.setFramerateLimit(140);
																if (customCursor) {
																	window.setMouseCursor(customCursor.value());
																}
															}
														}
														if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
														{
															if (mouseBtn->button == Mouse::Button::Left)
															{
																// Wywo≥anie funkcji przypisywania klawiszy 12 razy (na 12 przyciskÛw)
																keyBindingsFunction(window, clickSound, Jump1, keyJump1, L"SKOK", windowWidth / 2.0f - 500.0f, windowHeight / 2.0f - 250.0f, ControlsTitle, isFullscreen, windowWidth, windowHeight, mousePos);
																keyBindingsFunction(window, clickSound, Jump2, keyJump2, L"SKOK", windowWidth / 2.0f, windowHeight / 2.0f - 250.0f, ControlsTitle, isFullscreen, windowWidth, windowHeight, mousePos);
																keyBindingsFunction(window, clickSound, Jump3, keyJump3, L"SKOK", windowWidth / 2.0f + 500.0f, windowHeight / 2.0f - 250.0f, ControlsTitle, isFullscreen, windowWidth, windowHeight, mousePos);

																keyBindingsFunction(window, clickSound, Right1, keyRight1, L"PRAWO", windowWidth / 2.0f - 500.0f, windowHeight / 2.0f + 50.0f, ControlsTitle, isFullscreen, windowWidth, windowHeight, mousePos);
																keyBindingsFunction(window, clickSound, Right2, keyRight2, L"PRAWO", windowWidth / 2.0f, windowHeight / 2.0f + 50.0f, ControlsTitle, isFullscreen, windowWidth, windowHeight, mousePos);
																keyBindingsFunction(window, clickSound, Right3, keyRight3, L"PRAWO", windowWidth / 2.0f + 500.0f, windowHeight / 2.0f + 50.0f, ControlsTitle, isFullscreen, windowWidth, windowHeight, mousePos);

																keyBindingsFunction(window, clickSound, Left1, keyLeft1, L"LEWO", windowWidth / 2.0f - 500.0f, windowHeight / 2.0f - 150.0f, ControlsTitle, isFullscreen, windowWidth, windowHeight, mousePos);
																keyBindingsFunction(window, clickSound, Left2, keyLeft2, L"LEWO", windowWidth / 2.0f, windowHeight / 2.0f - 150.0f, ControlsTitle, isFullscreen, windowWidth, windowHeight, mousePos);
																keyBindingsFunction(window, clickSound, Left3, keyLeft3, L"LEWO", windowWidth / 2.0f + 500.0f, windowHeight / 2.0f - 150.0f, ControlsTitle, isFullscreen, windowWidth, windowHeight, mousePos);

																keyBindingsFunction(window, clickSound, Run1, keyRun1, L"BIEG", windowWidth / 2.0f - 500.0f, windowHeight / 2.0f - 50.0f, ControlsTitle, isFullscreen, windowWidth, windowHeight, mousePos);
																keyBindingsFunction(window, clickSound, Run2, keyRun2, L"BIEG", windowWidth / 2.0f, windowHeight / 2.0f - 50.0f, ControlsTitle, isFullscreen, windowWidth, windowHeight, mousePos);
																keyBindingsFunction(window, clickSound, Run3, keyRun3, L"BIEG", windowWidth / 2.0f + 500.0f, windowHeight / 2.0f - 50.0f, ControlsTitle, isFullscreen, windowWidth, windowHeight, mousePos);
															}
														}
														if (const auto* mouseBtn = event->getIf<Event::MouseButtonPressed>())
														{
															if (mouseBtn->button == Mouse::Button::Left)
															{
																// klikniÍcie ZRESETUJ
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
															clickSound.play();                                      // w≥πczenie/wy≥πczenie okreúlonych düwiÍkÛw
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
																if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "AGH survival", sf::Style::Default, sf::State::Fullscreen);
																else window.create(sf::VideoMode({ windowWidth, windowHeight }), "AGH survival", sf::Style::Default, sf::State::Windowed);
																window.setFramerateLimit(140);
																if (customCursor) {
																	window.setMouseCursor(customCursor.value());
																}
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
															expSound.setVolume(percent* volumeSfxScale * 0.3f);
															levelUpSound.setVolume(percent* volumeSfxScale * 1.2f);
															bossDeathSound.setVolume(percent* volumeSfxScale * 2.0f);
															bossHitSound.setVolume(percent* volumeSfxScale * 2.0f);
															hurtSound.setVolume(percent* volumeSfxScale * 1.2f);
															lightningSound.setVolume(percent* volumeSfxScale * 0.8f);
															whipSound.setVolume(percent* volumeSfxScale * 0.8f);
															beerSound.setVolume(percent* volumeSfxScale * 0.6f);
															pencilSound.setVolume(percent* volumeSfxScale * 0.7f);
															chairSound.setVolume(percent* volumeSfxScale);
															bananaSound.setVolume(percent* volumeSfxScale * 0.8f);
															stalSound.setVolume(percent* volumeSfxScale * 0.8f);
															flaskSound.setVolume(percent* volumeSfxScale * 0.8f);
															bibleHitSound.setVolume(percent* volumeSfxScale * 0.8f);
															garlicHitSound.setVolume(percent* volumeSfxScale * 0.5f);

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
																if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "AGH survival", sf::Style::Default, sf::State::Fullscreen);
																else window.create(sf::VideoMode({ windowWidth, windowHeight }), "AGH survival", sf::Style::Default, sf::State::Windowed);
																window.setFramerateLimit(140);
																if (customCursor) {
																	window.setMouseCursor(customCursor.value());
																}
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
																		window.create(VideoMode::getDesktopMode(), "AGH survival", Style::Default, sf::State::Fullscreen);
																		window.setFramerateLimit(140);
																		if (customCursor) {
																			window.setMouseCursor(customCursor.value());
																		}
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
																		window.create(VideoMode({ windowWidth, windowHeight }), "AGH survival", Style::Default, sf::State::Windowed);
																		window.setFramerateLimit(140);
																		if (customCursor) {
																			window.setMouseCursor(customCursor.value());
																		}
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
											if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "AGH survival", sf::Style::Default, sf::State::Fullscreen);
											else window.create(sf::VideoMode({ windowWidth, windowHeight }), "AGH survival", sf::Style::Default, sf::State::Windowed);
											window.setFramerateLimit(140);
											if (customCursor) {
												window.setMouseCursor(customCursor.value());
											}
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
							clickSound.play();
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
			window.draw(menuBgSprite);
			window.draw(menuTitle);
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
		bossBarFill.setPosition(sf::Vector2f((windowWidth / 2.f) - 400.f, 100.f));

		sf::Text bossNameText(font);
		bossNameText.setString("SAMOCHOD OCHRONY SPECJAL");
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
		player.setScale(sf::Vector2f(2.1f, 2.1f));

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
		std::vector<ActiveWeapon> weaponInventory;          // max 3 broÒ
		std::vector<ActivePassive> passiveInventory;        // max 3 przedmioty

		int level = 1;
		int exp = 0;
		int nextExp = 50;
		int killCount = 0;
		int magnetsCollected = 0;

		std::vector<Enemy> enemies;
		std::vector<Projectile> projectiles;
		std::vector<DamageZone> zones;
		std::vector<ExpOrb> expOrbs;
		std::vector<UpgradeCard> upgradeCards;

		sf::Clock dtClock;
		sf::Clock spawnClock;
		sf::Clock regenClock;
		float gameTime = 0.f;
		float activeMagnetTimer = 0.0f;

		sf::RectangleShape ground({ mapWidth, 100.f });
		ground.setFillColor(sf::Color::Transparent);
		ground.setPosition({ 0.f, windowHeight - 70.f });

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
						if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "AGH survival", sf::Style::Default, sf::State::Fullscreen);        // nazwa na pasku zadaÒ
						else window.create(sf::VideoMode({ windowWidth, windowHeight }), "AGH survival", sf::Style::Default, sf::State::Windowed);          //
						window.setFramerateLimit(140);
						if (customCursor) {
							window.setMouseCursor(customCursor.value());
						}
					}
				}
				// ZMIANA: Obs≥uga klikniÍcia jako ZDARZENIE wewnπtrz pÍtli pollEvent
				else if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>()) {                        //
					if (mouseBtn->button == sf::Mouse::Button::Left) {                                                  //      Funkcja dla klikniÍcia myszkπ 
						for (auto& btn : weaponButtons) {                                                               //
							if (btn.shape.getGlobalBounds().contains(mousePos)) {
								// Tutaj wybieramy broÒ
								weaponInventory.clear();
								WeaponDef* targetDef = nullptr;
								for (auto& dbW : weaponDB) if (dbW.id == btn.id) targetDef = &dbW;

								if (targetDef) {
									weaponInventory.push_back({ targetDef, 1, 0.f });
								}
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

			// --- RYSOWANIE EKRANU WYBORU BRONI (Z T£EM GRY) ---
			window.clear(sf::Color(20, 20, 20));

			// 1. Ustawienie kamery na gracza (øeby by≥o widaÊ t≥o)
			sf::View view(sf::FloatRect({ 0.f, 0.f }, { (float)windowWidth, (float)windowHeight }));
			float camX = std::clamp(player.getPosition().x, windowWidth / 2.f, mapWidth - windowWidth / 2.f);
			view.setCenter({ camX, windowHeight / 2.f });
			window.setView(view);

			// 2. Rysowanie úwiata (T≥o, Ziemia, Portale, Gracz)
			window.draw(background);
			window.draw(ground);
			for (auto& p : portals) window.draw(p.sprite);
			window.draw(player);

			// 3. PowrÛt do kamery UI
			window.setView(window.getDefaultView());

			// 4. Rysowanie przyciemnienia i Menu
			window.draw(overlay); // To daje efekt "pauzy"
			window.draw(title);

			for (auto& btn : weaponButtons) {
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
		while (window.isOpen() && gameStarted) {
			float dt = dtClock.restart().asSeconds();
			sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);

			while (const std::optional event = window.pollEvent()) {
				if (event->is<sf::Event::Closed>()) window.close();
				else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
					if (key->scancode == sf::Keyboard::Scancode::F11) {
						isFullscreen = !isFullscreen;
						window.close();
						if (isFullscreen) window.create(sf::VideoMode::getDesktopMode(), "AGH survival", sf::Style::Default, sf::State::Fullscreen);
						else window.create(sf::VideoMode({ windowWidth, windowHeight }), "AGH survival", sf::Style::Default, sf::State::Windowed);
						window.setFramerateLimit(140);
						dtClock.restart();
						if (customCursor) {
							window.setMouseCursor(customCursor.value());
						}
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
						inMainMenu = false;
						isDying = false;
						isHurt = false;
						isBraking = false;
						isTurning = false;

						// 1. Reset postÍpu
						level = 1;
						exp = 0;
						nextExp = 50;
						gameTime = 0.f;
						killCount = 0;
						magnetsCollected = 0;

						// 2. Czyszczenie wektorÛw
						enemies.clear();
						projectiles.clear();
						zones.clear();
						expOrbs.clear();
						upgradeCards.clear();
						weaponInventory.clear();
						passiveInventory.clear();
						bossSpawned = false;
						Boss.clear();
						Szczur.clear();
						Ochrona.clear();
						Egzamin.clear();

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
												weaponInventory.push_back({ &weaponDB[btn.id], 1, 0.f });
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

							// --- RYSOWANIE (IDENTYCZNE JAK NA STARCIE) ---
							window.clear(sf::Color(20, 20, 20));

							// Kamera na gracza
							sf::View view(sf::FloatRect({ 0.f, 0.f }, { (float)windowWidth, (float)windowHeight }));
							float camX = std::clamp(player.getPosition().x, windowWidth / 2.f, mapWidth - windowWidth / 2.f);
							view.setCenter({ camX, windowHeight / 2.f });
							window.setView(view);

							// åwiat
							window.draw(background);
							window.draw(ground);
							for (auto& p : portals) window.draw(p.sprite);
							window.draw(player);

							// UI
							window.setView(window.getDefaultView());
							window.draw(overlay); // Przyciemnienie
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
									if (w.def->id == weaponDB[idx].id) {
										// Sprawdzamy czy nie przekraczamy max levela
										if (w.level < w.def->levels.size()) {
											w.level++;
										}
										found = true;
										break;
									}
								}
								if (!found) weaponInventory.push_back({ &weaponDB[idx], 1, 0.f });
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
				std::string statsStr = "--- STATYSTYKI ---\n\n";

				statsStr += "Zabici wrogowie: " + std::to_string(killCount) + "\n\n";
				statsStr += "Zebrane magnesy: " + std::to_string(magnetsCollected) + "\n\n";

				statsStr += "Max HP: " + std::to_string(pStats.maxHp) + "\n";
				statsStr += "Regeneracja: " + std::to_string(pStats.regen).substr(0, 3) + "/s\n";
				statsStr += "Pancerz: " + std::to_string(pStats.armor) + "\n\n";

				// Mnoøymy razy 100, øeby pokazaÊ procenty (np. 1.1 -> 110%)
				statsStr += "Sila: " + std::to_string((int)(pStats.might * 100)) + "%\n";
				statsStr += "Obszar): " + std::to_string((int)(pStats.area * 100)) + "%\n";
				statsStr += "Szybkosc pocisku: " + std::to_string((int)(pStats.speed * 100)) + "%\n";
				statsStr += "Czas trwania: " + std::to_string((int)(pStats.duration * 100)) + "%\n";

				// Cooldown im mniejszy tym lepiej, wiÍc pokazujemy np. 90% (czyli -10%)
				statsStr += "Cooldown: " + std::to_string((int)(pStats.cooldown * 100)) + "%\n";

				statsStr += "Ilosc: +" + std::to_string(pStats.amount) + "\n";
				statsStr += "Zasieg: " + std::to_string((int)pStats.magnet) + "\n";

				pauseStatsText.setString(statsStr);
				// --- 1. OBS£UGA RESTARTU ---
				if (pauseRestart.getGlobalBounds().contains(mousePos)) {
					pauseRestart.setFillColor(sf::Color::Red);

					if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
						clickSound.play();
						// Logika Restartu:
						isPaused = false;
						gameStarted = false; // Przerywamy pÍtlÍ gry, wracamy do pÍtli "Matki"
						inMainMenu = false;  // Ustawiamy false, øeby pÍtla Matka pominÍ≥a Menu i wesz≥a do Wyboru Broni

						// Reset zegarÛw i muzyki
						spawnClock.restart();
						regenClock.restart();
						magnetsCollected = 0;
						killCount = 0;
						bgMusic.stop();
					}
				}
				else {
					pauseRestart.setFillColor(sf::Color::White);
				}

				// --- 2. OBS£UGA MENU G£”WNEGO ---
				if (pauseMainMenu.getGlobalBounds().contains(mousePos)) {
					pauseMainMenu.setFillColor(sf::Color::Red);

					if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
						clickSound.play();
						// Logika Wyjúcia do Menu:
						isPaused = false;
						gameStarted = false; // Przerywamy pÍtlÍ gry
						inMainMenu = true;   // Ustawiamy true, øeby pÍtla Matka wesz≥a do Menu G≥Ûwnego

						bgMusic.stop();
					}
				}
				else {
					pauseMainMenu.setFillColor(sf::Color::White);
				}
			}
			else if (!isPaused) {
				gameTime += dt;
				// KONIEC GRY PO 6 MINUTACH ---
				if (gameTime >= 300.0f) {
					isGameOver = true;

					// Zatrzymujemy muzykÍ i grÍ
					bgMusic.stop();
					levelUpSound.play(); // Moøesz uøyÊ levelUpSound jako fanfary zwyciÍstwa

					// Zmieniamy napis z "GAME OVER" na "ZWYCI STWO"
					gameOverTitle.setString("ZWYCIESTWO!");
					gameOverTitle.setFillColor(sf::Color::Green);

					// Centrujemy nowy napis (bo ma innπ d≥ugoúÊ niø Game Over)
					centerText(gameOverTitle, windowWidth / 2.f, windowHeight / 2.f - 100.f);
				}
				// ------------------------------------------

				// --- REGENERACJA HP ---
				if (pStats.regen > 0.f) {
					if (regenClock.getElapsedTime().asSeconds() >= 1.0f) {// Sprawdzamy, czy minÍ≥a 1 sekunda
						if (currentHp < pStats.maxHp) {
							currentHp += pStats.regen;
							if (currentHp > pStats.maxHp) {// Upewniamy siÍ, øe nie przekroczymy Max HP
								currentHp = (float)pStats.maxHp;
							}
						}
						regenClock.restart(); // Resetujemy zegar regeneracji
					}
				}

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
						frameDuration = 0.04f; // ObrÛt jest doúÊ szybki
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

				if (!isDying) {
					float currentSpeed = pStats.moveSpeed;
					if (sf::Keyboard::isKeyPressed(keyRun1) || sf::Keyboard::isKeyPressed(keyRun2) || sf::Keyboard::isKeyPressed(keyRun3)) {
						currentSpeed *= 1.7f;
					}
					bool pressingLeft =
						(keyLeft1 != Keyboard::Scancode::Unknown && sf::Keyboard::isKeyPressed(keyLeft1)) ||
						(keyLeft2 != Keyboard::Scancode::Unknown && sf::Keyboard::isKeyPressed(keyLeft2)) ||
						(keyLeft3 != Keyboard::Scancode::Unknown && sf::Keyboard::isKeyPressed(keyLeft3));

					bool pressingRight =
						(keyRight1 != Keyboard::Scancode::Unknown && sf::Keyboard::isKeyPressed(keyRight1)) ||
						(keyRight2 != Keyboard::Scancode::Unknown && sf::Keyboard::isKeyPressed(keyRight2)) ||
						(keyRight3 != Keyboard::Scancode::Unknown && sf::Keyboard::isKeyPressed(keyRight3));

					if (pressingLeft && !pressingRight) {
						velocity.x -= currentSpeed;

						if (facingDir == 1 && !isTurning && onGround) {
							isTurning = true; isBraking = false; currentAnimFrame = 0; animTimer = 0.f;
						}
						else if (!isTurning) facingDir = -1;
					}
					else if (pressingRight && !pressingLeft) {
						velocity.x += currentSpeed;

						if (facingDir == -1 && !isTurning && onGround) {
							isTurning = true; isBraking = false; currentAnimFrame = 0; animTimer = 0.f;
						}
						else if (!isTurning) facingDir = 1;
					}
					bool pressingJump =
						(keyJump1 != Keyboard::Scancode::Unknown && sf::Keyboard::isKeyPressed(keyJump1)) ||
						(keyJump2 != Keyboard::Scancode::Unknown && sf::Keyboard::isKeyPressed(keyJump2)) ||
						(keyJump3 != Keyboard::Scancode::Unknown && sf::Keyboard::isKeyPressed(keyJump3));

					if (pressingJump && onGround) {
						velocity.y = jumpForce;
						onGround = false;
						isTurning = false;
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

				// --- SPAWNOWANIE PRZECIWNIK”W  ---
				float spawnDelay = 1.0f - (gameTime * 0.0039f);
				if (spawnDelay < 0.06f) spawnDelay = 0.06f;

				if (spawnClock.getElapsedTime().asSeconds() > spawnDelay) {
						// --- KOD BOSSA ---
						if (!bossSpawned && gameTime >= 210.f) {// 3.5minuty
							Enemy boss;
							boss.id = nextEnemyId++;
							boss.hp = 3000.0f;
							boss.maxHp = 3000.0f;
							boss.isBoss = true;

							// Wyglπd bossa (Duøy i inny kolor)
							boss.shape.setRadius(100.f);
							boss.shape.setFillColor(sf::Color::Transparent);
							boss.shape.setOrigin(sf::Vector2f(100.f, 100.f));

							float bossSpawnX = 0.f;

							// Losujemy portal: 0 = Lewy, 1 = Prawy
							if (rand() % 2 == 0) {
								bossSpawnX = 80.f; // Pozycja lewego portalu
							}
							else {
								bossSpawnX = mapWidth - 80.f; // Pozycja prawego portalu
							}

							// WysokoúÊ (Y)
							// Ustawiamy go na ziemi. Odejmujemy 80, øeby duøy boss nie zapada≥ siÍ w pod≥ogÍ.
							float bossSpawnY = ground.getPosition().y - 80.f;

							boss.shape.setPosition(sf::Vector2f(bossSpawnX, bossSpawnY));

							// --- DODAJEMY ANIMACJ  ---
							Boss.emplace_back(bossTexture, boss.shape.getPosition());
							Boss.back().targetId = boss.id;
							// -------------------------

							enemies.push_back(boss);
							bossSpawned = true; // Zablokuj ponowne spawnowanie

							// Nie spawnuj zwyk≥ego wroga w tej samej klatce co bossa
							spawnClock.restart();
						}
					// ------------------------

					Enemy e;
					e.id = nextEnemyId++;

					// 1. WYB”R TYPU PRZECIWNIKA (Zaleønie od czasu)
					int enemyType = 0; // 0 = S≥aby (Domyúlny)
					int randVal = rand() % 100; // Losowa liczba 0-99

					if (gameTime >= 240.f) {  // Po 4 minutach
						// 0% S≥abych
						// 60% årednich (Ochrona)
						// 40% Mocnych (Egzamin)
						if (randVal < 50) {
							enemyType = 1; // åredni
						}
						else {
							enemyType = 2; // Mocny
						}
					}
					else if (gameTime >= 120.f) { // Po 2 minutach
						// 50% S≥aby, 30% åredni, 20% Silny
						if (randVal < 50) enemyType = 0;
						else if (randVal < 80) enemyType = 1;
						else enemyType = 2;
					}
					else if (gameTime >= 30.f) { // Po 30 sekundach
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
						e.shape.setFillColor(sf::Color::Transparent);
						Szczur.emplace_back(szczurTexture, e.shape.getPosition());
						Szczur.back().targetId = e.id;
					}
					else if (enemyType == 1) { // åredni
						e.hp = 25.0f; e.maxHp = 25.0f;
						e.shape.setRadius(30.f);
						e.shape.setFillColor(sf::Color::Transparent);
						Ochrona.emplace_back(ochronaTexture, e.shape.getPosition());
						Ochrona.back().targetId = e.id;
						// Opcjonalnie wolniejszy? (logika prÍdkoúci jest niøej w pÍtli ruchu)
					}
					else if (enemyType == 2) { // Twardy
						e.hp = 60.0f; e.maxHp = 60.0f;
						e.shape.setRadius(35.f);
						e.shape.setFillColor(sf::Color::Transparent);
						Egzamin.emplace_back(egzaminTexture, e.shape.getPosition());
						Egzamin.back().targetId = e.id;
					}

					e.shape.setOrigin(sf::Vector2f(e.shape.getRadius(), e.shape.getRadius()));

					// 3. BEZPIECZNA POZYCJA spawn tylko z lewej i prawej strony ekranu
					float spawnX = 0.f;
					if (rand() % 2 == 0) {// Losujemy stronÍ: 0 = Lewa, 1 = Prawa
						spawnX = 80.f; // TUTAJ DOPASOWAC DO MIEJSCA W KTORYM JEST PORTAL
					}
					else {
						spawnX = mapWidth - 80; // TUTAJ DOPASOWAC DO MIEJSCA W KTORYM JEST PORTAL
					}

					float spawnY = windowHeight - 100.f;

					e.shape.setPosition(sf::Vector2f(spawnX, spawnY));

					enemies.push_back(e);
					spawnClock.restart();
				}
				// 3. BRONIE (STRZELANIE)
				for (auto& w : weaponInventory) {
					w.cooldownTimer -= dt;

					// 1. POBIERZ STATYSTYKI DLA OBECNEGO POZIOMU
					// w.level - 1, bo wektor liczymy od 0 (lvl 1 to indeks 0)
					const WeaponLevelStats& stats = w.def->levels[w.level - 1];

					// Czosnek
					if (w.def->id == GARLIC) {
						// Obliczamy aktualny promieÒ w kaødej klatce
						// 100.f to baza, stats.area to bonus z poziomu broni, pStats.area to pasywka (Candle)
						float currentRadius = 100.f * stats.area * pStats.area;

						bool zoneExists = false;
						for (auto& z : zones) {
							if (z.wId == GARLIC) {
								zoneExists = true;

								// AKTUALIZACJA ISTNIEJ•CEJ STREFY
								// DziÍki temu po wziÍciu pasywki strefa uroúnie "na øywo"
								if (z.shape.getRadius() != currentRadius) {
									z.shape.setRadius(currentRadius);
									z.shape.setOrigin({ currentRadius, currentRadius });
								}

								// Aktualizujemy teø obraøenia (np. po wziÍciu Szpinaku)
								z.damage = stats.damage * pStats.might;
							}
						}

						// Jeúli strefa nie istnieje (np. poczπtek gry), stwÛrz jπ
						if (!zoneExists) {
							DamageZone z;
							z.wId = GARLIC;
							z.maxLifeTime = 99999.f;
							z.damage = stats.damage * pStats.might;

							z.shape.setRadius(currentRadius);
							z.shape.setFillColor(sf::Color(0, 150, 0, 100));
							z.shape.setOrigin({ currentRadius, currentRadius });
							zones.push_back(z);
						}
						continue; // Przechodzimy do nastÍpnej broni, bo Czosnek nie strzela pociskami
					}

					if (w.cooldownTimer <= 0.f) {
						// Uøywamy stats.cooldown
						w.cooldownTimer = stats.cooldown * pStats.cooldown;

						float sizeMult = stats.area * pStats.area;
						// Logika Amount (Baza z poziomu + Bonus gracza)
						// Np. Whip lvl 3 ma w bazie amount=2.
						int amount = stats.amount + pStats.amount;
						if (w.def->id == WHIP) {
							szalik.emplace_back(szalikTexture, player.getPosition(), facingDir, 1, sizeMult);

							if (amount > 1) {
								szalik.emplace_back(szalikTexture, player.getPosition(), facingDir, -1, sizeMult);
							}
						}


						for (int i = 0; i < amount; ++i) {
							Projectile p;
							p.wId = w.def->id;

							// --- UØYWAMY STATYSTYK Z POZIOMU ---
							p.damage = stats.damage * pStats.might;
							p.maxLifeTime = stats.duration * pStats.duration;
							float speed = stats.speed * pStats.speed;
							p.baseSpeed = speed;
							p.pierceLeft = stats.pierce;
							// -----------------------------------


							float minDist = 10000.f;
							sf::Vector2f target = player.getPosition() + sf::Vector2f(100, 0);
							float rad = 10.f * sizeMult;
							p.shape.setRadius(rad);
							p.shape.setFillColor(sf::Color::White);
							p.shape.setOrigin({ rad, rad });
							p.shape.setPosition(player.getPosition());

							float pitchVar = 0.9f + (float)(rand() % 20) / 100.f;
							// --- KONFIGURACJA KSZTA£TU (WHIP) ---
							if (w.def->id == WHIP) {
								whipSound.setPitch(pitchVar);
								whipSound.play();

								p.boxShape.setSize({ 100.f * sizeMult, 30.f * sizeMult });

								// --- 2. ZMIE— TE DWIE LINIKI: ---
								p.boxShape.setTexture(nullptr);         // Usuwamy teksturÍ z hitboxa
								p.boxShape.setFillColor(sf::Color::Transparent); // Robimy go niewidzialnym
								// --------------------------------

								p.boxShape.setOrigin({ 0.f, 20.f * sizeMult });
								p.startPos = player.getPosition();
								p.boxShape.setPosition(player.getPosition());

								// Ustalmy kierunek hitboxa (1 = prawo, -1 = lewo)
								float scaleDir = (float)facingDir;

								// Jeúli to parzysty indeks (i=1, 3...), to jest to "tylny" atak, wiÍc odwracamy kierunek
								if (i % 2 != 0) {
									scaleDir = -scaleDir;
								}

								// Teraz ustawiamy skalÍ: 1.5 szerokoúci w odpowiedniπ stronÍ
								p.boxShape.setScale({ 1.5f * scaleDir, 1.5f });
							}
							// --- LIGHTNING ---
							else if (w.def->id == LIGHTNING) {
								if (enemies.empty()) continue;

								std::vector<int> validTargets;
								for (size_t k = 0; k < enemies.size(); ++k) {
									if (vectorLength(enemies[k].shape.getPosition() - player.getPosition()) <= 800.f) {
										validTargets.push_back(k);
									}
								}
								if (validTargets.empty()) {
									w.cooldownTimer = 0.05f;
									continue;
								}

								int idx = validTargets[rand() % validTargets.size()];

								// ODTWARZANIE DèWI KU PIORUNA
								lightningSound.setPitch(pitchVar);
								lightningSound.play();

								//  TWORZENIE ANIMOWANEGO PIORUNA
								lightnings.emplace_back(piorunTexture, enemies[idx].shape.getPosition());
								p.boxShape.setSize({ 40.f, 100.f });
								p.boxShape.setOrigin({ 20.f, 100.f });
								p.boxShape.setPosition(enemies[idx].shape.getPosition());

								// Ustawiamy czas øycia hitboxa
								p.maxLifeTime = 0.2f;
							}
							// --- INNE BRONIE --- 
							else if (w.def->id == MAGIC_WAND) {
								beerSound.setPitch(pitchVar);
								beerSound.play();

								p.shape.setTexture(&piwoTexture);
								p.shape.setScale({ 1.7f, 1.7f });
								for (auto& e : enemies) {
									float d = vectorLength(e.shape.getPosition() - player.getPosition());
									if (d < minDist) { minDist = d; target = e.shape.getPosition(); }
								}
								sf::Vector2f dir = normalize(target - player.getPosition());
								p.velocity = dir * speed;

								// LOGIKA W ØYKA: Przesuwamy start do ty≥u dla kolejnych pociskÛw
								// i=0 (0px), i=1 (-20px), i=2 (-40px) wzd≥uø wektora lotu
								p.shape.setPosition(player.getPosition() - (dir * ((float)i * 25.f)));
								p.shape.rotate(sf::degrees(360.f * dt));
							}
							else if (w.def->id == KNIFE) {
								pencilSound.setPitch(pitchVar);
								pencilSound.play();

								sf::Vector2f dir((float)facingDir, 0.f);
								p.velocity = normalize(dir) * speed;

								// LOGIKA W ØYKA (jeden za drugim)
								p.shape.setPosition(player.getPosition() - (dir * ((float)i * 30.f)));
								p.shape.setTexture(&olowekTexture);                      // przypisz tekstury

								// Obracanie w zaleønoúci od kierunku
								if (facingDir == -1) p.shape.setScale({ -2.f, 2.f });
								else p.shape.setScale({ 2.f, 2.f });
							}
							else if (w.def->id == AXE) {
								chairSound.setPitch(pitchVar);
								chairSound.play();

								float fixedThrowPowerY = 1100.f;
								float fixedThrowPowerX = 350.f;

								float xDir = (float)facingDir * fixedThrowPowerX + (i * 100.f * facingDir);

								// Ustawiamy wektor startowy "na sztywno"
								p.velocity = sf::Vector2f(xDir, -fixedThrowPowerY - (i * 50.f));
								p.baseSpeed = speed / 600.f;
								p.shape.setTexture(&krzesloTexture);
								p.shape.setScale({ 3.5f, 3.5f });

							}
							else if (w.def->id == BOOMERANG) {
								// DüwiÍk tylko dla pierwszego banana (øeby nie og≥uszyÊ)
								if (i == 0) {
									float pVar = 0.9f + (float)(rand() % 20) / 100.f;
									bananaSound.setPitch(pVar);
									bananaSound.play();
								}

								p.startPos = player.getPosition();

								// 1. Szukamy najbliøszego celu
								float minDist = 10000.f;
								sf::Vector2f target = player.getPosition() + sf::Vector2f(100 * facingDir, 0); // Domyúlnie prosto

								for (auto& e : enemies) {
									float d = vectorLength(e.shape.getPosition() - player.getPosition());
									if (d < minDist) {
										minDist = d;
										target = e.shape.getPosition();
									}
								}

								// 2. Obliczamy kierunek
								sf::Vector2f dir = normalize(target - player.getPosition());

								// Ustawiamy prÍdkoúÊ
								p.velocity = dir * speed;

								// 3. EFEKT "JEDEN ZA DRUGIM"
								// Cofamy pozycjÍ startowπ kolejnych bananÛw.
								// 40.f to odstÍp w pikselach miÍdzy bananami.
								p.shape.setPosition(player.getPosition() - (dir * ((float)i * 45.f)));

								p.shape.setTexture(&bananTexture);
								p.shape.setScale({ 2.0f, 2.0f });
								}
							else if (w.def->id == BIBLE) {
								// LOGIKA BIBLII: Rozk≥adamy rÛwno po kole
								// i=0 (0st), i=1 (180st) dla amount=2
								float angleStep = 360.f / (float)amount; // Np. 360/2 = 180
								p.angleOffset = (float)i * angleStep;
								p.shape.setTexture(&ksiazkaTexture);
								p.shape.setScale({ 2.0f, 2.0f });
							}
							else if (w.def->id == FIRE_WAND) {
								float angle = (float)(rand() % 360) * 3.14f / 180.f;
								p.velocity = sf::Vector2f(std::cos(angle), std::sin(angle)) * speed;
								p.shape.setTexture(&stalTexture);
								p.shape.setScale({ 2.5f, 2.5f });
								stalSound.setPitch(pitchVar);
								stalSound.play();
							}
							else if (w.def->id == HOLY_WATER) {
								float angle = (float)(rand() % 360);
								p.velocity = sf::Vector2f(std::cos(angle), std::sin(angle)) * speed;
								p.maxLifeTime = 1.0f;
								p.shape.setTexture(&potkaTexture);
								p.shape.setScale({ 2.0f, 2.0f });
								flaskSound.setPitch(pitchVar);
								flaskSound.play();
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

					if (p.lifeTime > p.maxLifeTime) {
						if (p.wId == HOLY_WATER) {
							DamageZone z;
							z.wId = HOLY_WATER;
							z.maxLifeTime = 3.0f * pStats.duration;
							z.damage = p.damage;
							float r = 60.f * pStats.area;
							z.shape.setRadius(r);
							z.shape.setFillColor(sf::Color(0, 0, 255, 100));
							z.shape.setOrigin({ r, r });
							z.shape.setPosition(p.shape.getPosition());
							zones.push_back(z);
						}
						dead = true;
					}

					if (!dead) {
						float pitchVar = 0.9f + (float)(rand() % 20) / 100.f;
						// Logika ruchu (bicz, axe, bible...)
						if (p.wId == AXE) {
							// "AXE TIME": Mnoøymy czas przez baseSpeed.
							// Jeúli baseSpeed = 2.0, to w jednej klatce gry (dt)
							// topÛr wykona ruch, jakby minÍ≥y dwie klatki.
							float axeDt = dt * p.baseSpeed;

							// Uøywamy axeDt zamiast zwyk≥ego dt do grawitacji i ruchu
							p.velocity.y += 2500.f * axeDt; // Grawitacja (taka sama jak baseGravity)
							p.shape.move(p.velocity * axeDt);

							// Obracanie
							p.shape.rotate(sf::degrees(360.f * axeDt));
						}
						else if (p.wId == BOOMERANG) {
							p.shape.move(p.velocity * dt);
							p.shape.rotate(sf::degrees(720.f * dt));
							if (!p.returning && p.lifeTime > p.maxLifeTime * 0.4f) { p.velocity = -p.velocity; p.returning = true; }
						}
						else if (p.wId == MAGIC_WAND) {
							p.shape.move(p.velocity * dt);
							p.shape.rotate(sf::degrees(360.f * dt));
						}
						else if (p.wId == BIBLE) {
							float currentSpeed = p.baseSpeed;
							float dist = 120.f * pStats.area;
							// Obliczamy kπt w stopniach: Czas gry + PrzesuniÍcie dla tego konkretnego pocisku
							float baseAngle = gameTime * currentSpeed * 57.29f; // * 57.29 zamienia radiany na stopnie (opcjonalne, zaleønie jak liczysz speed)
							// Proúciej:
							float finalAngleDegrees = (gameTime * 200.f) + p.angleOffset;

							// Zamiana na radiany do sin/cos
							float rad = finalAngleDegrees * 3.14159f / 180.f;

							p.shape.setPosition(player.getPosition() + sf::Vector2f(std::cos(rad) * dist, std::sin(rad) * dist));
						}
						else if (p.wId == WHIP) { p.boxShape.setPosition(player.getPosition()); }
						else { p.shape.move(p.velocity * dt); }

						// Kolizje
						sf::FloatRect pBounds;

						if (p.wId == WHIP || p.wId == LIGHTNING) {
							pBounds = p.boxShape.getGlobalBounds();
						}
						else {
							pBounds = p.shape.getGlobalBounds();
						}

						for (auto& en : enemies) {
							bool hitAlready = false; 
							for (int id : p.hitEnemies) if (id == en.id) hitAlready = true;
							if (hitAlready) continue;

							// KOLIZJA POCISKU Z WROGIEM
							if (pBounds.findIntersection(en.shape.getGlobalBounds())) {
								bool hitAlready = false;
								for (int id : p.hitEnemies) if (id == en.id) hitAlready = true;
								if (hitAlready) continue;

								p.hitEnemies.push_back(en.id);

								if (p.wId == BIBLE) {
									float pi = 0.9f + (float)(rand() % 20) / 100.f;
									bibleHitSound.setPitch(pi);
									bibleHitSound.play();
								}

								// 1. ZADAJ OBRAØENIA (bezpoúrednie trafienie fiolkπ)
								en.hp -= p.damage;

								// 2. SPRAWDè CZY WR”G ØYJE
								if (en.hp <= 0.f) {
									killCount++;

									sf::Vector2f deathPos = en.shape.getPosition();
									en.shape.setPosition(sf::Vector2f(-9000.f, -9000.f));

									ExpOrb orb(expOrbTexture);
									orb.sprite.setTextureRect(sf::IntRect({ 0, 0 }, { 16, 16 }));
									orb.sprite.setOrigin(sf::Vector2f(8.f, 8.f));
									orb.sprite.setPosition(deathPos);

									float randX = (float)(rand() % 200 - 100);
									orb.velocity = { randX, -400.f };

									// --- TUTAJ USTALAMY WARTOå∆ XP ---
									if (en.isBoss) {
										orb.value = 10000;
										orb.sprite.setColor(sf::Color::Red);
										bossDeathSound.play();
									}
									else {
										orb.value = 10; // Zwyk≥y szczur
									}
									// ---------------------------------
									if (rand() % 500 == 0) {                                                                                  //szansa na magnes 1 do 500

										ExpOrb mag(magnesTexture);
										mag.isMagnet = true;

										// Resetujemy kolor na bia≥y
										mag.sprite.setColor(sf::Color::White);

										// Ustawiamy wymiary (bezpiecznie)
										sf::Vector2u texSize = magnesTexture.getSize();
										if (texSize.x > 0 && texSize.y > 0) {
											mag.sprite.setTextureRect(sf::IntRect({ 0, 0 }, { (int)texSize.x, (int)texSize.y }));
											mag.sprite.setOrigin(sf::Vector2f(texSize.x / 2.f, texSize.y / 2.f));
										}
										else {
											// Zabezpieczenie gdyby tekstura jednak nie zadzia≥a≥a
											mag.sprite.setTexture(expOrbTexture);
											mag.sprite.setTextureRect(sf::IntRect({ 0, 0 }, { 16, 16 }));
											mag.sprite.setOrigin({ 8.f, 8.f });
											mag.sprite.setColor(sf::Color::Magenta);
										}

										mag.sprite.setPosition(deathPos);

										// Wyrzucamy magnes mocno w gÛrÍ
										mag.velocity = { 0.f, -800.f };

										expOrbs.push_back(mag);
									}

									expOrbs.push_back(orb);
								}
								else {
									// Odrzut (opcjonalny)
									en.shape.move(p.velocity * dt * 0.5f);
								}

								// --- FLASK (HOLY WATER) ---
								// Jeúli fiolka trafi wroga, musi pÍknπÊ i stworzyÊ strefÍ!
								if (p.wId == HOLY_WATER) {
									DamageZone z;
									z.wId = HOLY_WATER;
									z.maxLifeTime = 3.0f * pStats.duration;
									z.damage = p.damage; // Obraøenia strefy takie same jak pocisku
									float r = 60.f * pStats.area;
									z.shape.setRadius(r);
									z.shape.setFillColor(sf::Color(0, 0, 255, 100));
									z.shape.setOrigin({ r, r });
									z.shape.setPosition(p.shape.getPosition()); // Strefa w miejscu trafienia wroga
									zones.push_back(z);

									dead = true; // Natychmiastowe zniszczenie pocisku
									break;       // Przerywamy pÍtlÍ wrogÛw dla tego pocisku (bo juø nie istnieje)
								}
								// ----------------------------------

								if (p.pierceLeft > 0) p.pierceLeft--;
								else { dead = true; break; }
							}
						}
					}
					if (dead) projectiles.erase(projectiles.begin() + i); else i++;
				}

				// 5. STREFY OBRAØE— (Garlic, Holy Water)
				for (size_t i = 0; i < zones.size();) {
					DamageZone& z = zones[i];
					z.lifeTime += dt; z.tickTimer += dt;

					if (z.wId == GARLIC) z.shape.setPosition(player.getPosition());

					if (z.tickTimer > 0.2f) {
						z.tickTimer = 0.f;
						bool hitSoundPlayed = false;
						for (auto& en : enemies) {
							if (z.shape.getGlobalBounds().findIntersection(en.shape.getGlobalBounds())) {

								if (z.wId == GARLIC && !hitSoundPlayed) {
									float pi = 0.9f + (float)(rand() % 20) / 100.f;
									garlicHitSound.setPitch(pi);
									garlicHitSound.play();

									hitSoundPlayed = true; // Zablokuj düwiÍk dla reszty wrogÛw w tej klatce
								}

								// 1. ZADAJ OBRAØENIA
								en.hp -= z.damage;

								// 2. SPRAWDè CZY UMAR£
								if (en.hp <= 0.f) {
									killCount++;
									sf::Vector2f deathPos = en.shape.getPosition();
									en.shape.setPosition(sf::Vector2f(-9000.f, -9000.f));

									ExpOrb orb(expOrbTexture);
									orb.sprite.setTextureRect(sf::IntRect({ 0, 0 }, { 16, 16 }));
									orb.sprite.setOrigin(sf::Vector2f(8.f, 8.f));
									orb.sprite.setPosition(deathPos);

									float randX = (float)(rand() % 200 - 100);
									orb.velocity = { randX, -400.f };

									if (en.isBoss) {
										orb.value = 10000;
										orb.sprite.setColor(sf::Color::Red);
										bossDeathSound.play();
									}
									else {
										orb.value = 10;
									}

									expOrbs.push_back(orb);
								}
							}
						}
					}
					if (z.lifeTime > z.maxLifeTime) zones.erase(zones.begin() + i); else i++;
				}

				// 6. RUCH WROG”W
				for (size_t i = 0; i < enemies.size();) {
					if (isDying) {
						i++;
						continue;
					}
					if (enemies[i].shape.getPosition().x < -10000) { enemies.erase(enemies.begin() + i); continue; }

					// Celujemy w hitbox
					sf::Vector2f dir = normalize(hitbox.getPosition() - enemies[i].shape.getPosition());

					if (hitbox.getPosition().x < enemies[i].shape.getPosition().x) enemies[i].velocity.x = -150.f;
					else enemies[i].velocity.x = 150.f;

					enemies[i].velocity.y += baseGravity * dt;
					enemies[i].shape.move(enemies[i].velocity * dt);

					// Domyúlne wartoúci dla zwyk≥ych wrogÛw (Szczur/Ochrona)
					float enemyBottomOffset = 25.f; // Po≥owa wysokoúci wroga
					float flyHeight = 0.f;          // Ile nad ziemiπ ma wisieÊ

					// Jeúli to Boss, zmieniamy parametry
					if (enemies[i].isBoss) {
						enemyBottomOffset = 0.f;
						flyHeight = 50.f; 
					}

					// Sprawdzamy kolizjÍ z "wirtualnπ pod≥ogπ" (prawdziwa ziemia minus wysokoúÊ latania)
					if (enemies[i].shape.getPosition().y + enemyBottomOffset > ground.getPosition().y - flyHeight) {
						enemies[i].shape.setPosition({
							enemies[i].shape.getPosition().x,
							ground.getPosition().y - flyHeight - enemyBottomOffset });
						enemies[i].velocity.y = 0.f;
					}

					// ZMIANA: Obs≥uga obraøeÒ i úmierci
					if (enemies[i].shape.getGlobalBounds().findIntersection(hitbox.getGlobalBounds()) && !isDying) {

						float dmg = 1.0f; // Domyúlne obraøenia (Szczur)

						// --- SYSTEM OBRAØE— WED£UG TYPU WROGA ---
						if (enemies[i].isBoss) {
							dmg = 100.0f; // Boss
							bossHitSound.play();
						}
						else if (enemies[i].maxHp == 100.0f) {
							dmg = 5.0f;   // Enemy 3 (Egzamin)
							hurtSound.play();
						}
						else if (enemies[i].maxHp == 15.0f) {
							dmg = 3.0f;   // Enemy 2 (Ochrona)
							hurtSound.play();
						}
						else {
							dmg = 1.0f;   // Enemy 1 (Szczur)
							hurtSound.play();
						}

						// Odejmujemy pancerz gracza
						if (pStats.armor > 0) dmg = dmg - (int)pStats.armor;

						// Minimalne obraøenia to zawsze 0.1 (øeby pancerz nie dawa≥ nieúmiertelnoúci)
						if (dmg < 0.1f) dmg = 0.1f;

						currentHp -= dmg;

						// Logika usuwania wroga
						if (!enemies[i].isBoss) {
							// Zwyk≥y wrÛg znika po trafieniu
							enemies.erase(enemies.begin() + i);
						}
						else {
							i++;
						}

						// Obs≥uga úmierci gracza
						if (currentHp <= 0) {
							isDying = true;
							isHurt = false;
							bgMusic.stop();
							gameOverSound.play();
						}
						else {
							isHurt = true;
						}
					}
					else {
						i++;
					}
				}

				// 7. EXP ORBS
				if (activeMagnetTimer > 0.0f) {
					activeMagnetTimer -= dt;
				}

				for (size_t i = 0; i < expOrbs.size();) {

					// --- POPRAWKA: BLOKADA ANIMACJI DLA MAGNESU ---
					// Animujemy tylko wtedy, gdy to NIE jest magnes (!isMagnet)
					if (!expOrbs[i].isMagnet) {
						expOrbs[i].animTimer += dt;
						if (expOrbs[i].animTimer >= 0.1f) {
							expOrbs[i].animTimer = 0.f;
							expOrbs[i].currentFrame = (expOrbs[i].currentFrame + 1) % 4;
							expOrbs[i].sprite.setTextureRect(sf::IntRect({ expOrbs[i].currentFrame * 16, 0 }, { 16, 16 }));
						}
					}
					// ---------------------------------------------

					// --- B. LOGIKA PRZYCI•GANIA ---
					float currentMagnetRange = pStats.magnet;

					// Jeúli aktywny super-magnes, zasiÍg jest ogromny (ca≥a mapa)
					if (activeMagnetTimer > 0.0f) {
						currentMagnetRange = 999999.0f;
					}

					sf::Vector2f dirToP = hitbox.getPosition() - expOrbs[i].sprite.getPosition();
					float dist = vectorLength(dirToP);

					if (dist < currentMagnetRange) {
						float pullSpeed = 1200.f;
						if (activeMagnetTimer > 0.0f) pullSpeed = 2000.f;

						expOrbs[i].velocity = normalize(dirToP) * pullSpeed;
						expOrbs[i].isOnGround = false;
					}
					else if (!expOrbs[i].isOnGround) {
						expOrbs[i].velocity.y += 1500.f * dt; // Grawitacja
					}

					expOrbs[i].sprite.move(expOrbs[i].velocity * dt);

					// Kolizja z ziemiπ
					if (expOrbs[i].sprite.getPosition().y + 5.f > ground.getPosition().y) {
						expOrbs[i].sprite.setPosition({ expOrbs[i].sprite.getPosition().x, ground.getPosition().y - 5.f });
						expOrbs[i].velocity.y = 0.f; expOrbs[i].velocity.x = 0.f; expOrbs[i].isOnGround = true;
					}

					// --- C. PODNOSZENIE ---
					if (expOrbs[i].sprite.getGlobalBounds().findIntersection(hitbox.getGlobalBounds())) {

						// SPRAWDZAMY CZY TO MAGNES CZY XP
						if (expOrbs[i].isMagnet) {
							// --- EFEKT MAGNESU ---
							activeMagnetTimer = 3.0f; // Aktywuj na 3 sekundy
							magnetsCollected++;

							levelUpSound.setPitch(2.0f);
							levelUpSound.play();
						}
						else {
							// --- ZWYK£Y XP ---
							float randomPitch = 0.8f + (float)(rand() % 40) / 100.f;
							expSound.setPitch(randomPitch);
							expSound.play();

							exp += expOrbs[i].value;

							// Sprawdzanie Level Up
							if (exp >= nextExp) {
								levelUpSound.setPitch(1.0f);
								levelUpSound.play();
								exp -= nextExp;
								nextExp = (int)(nextExp * 1.2f);
								level++;

								// GENEROWANIE KART LEVEL UP
								upgradeCards.clear();

								std::vector<std::pair<int, int>> pool;

								if (weaponInventory.size() < 3) {
									for (int k = 0; k < (int)weaponDB.size(); ++k) {
										bool has = false;
										for (auto& w : weaponInventory) if (w.def->id == weaponDB[k].id) has = true;
										if (!has) pool.push_back({ 0, k });
									}
								}
								for (auto& w : weaponInventory) {
									if (w.level < 7) {
										for (int k = 0; k < (int)weaponDB.size(); ++k) if (weaponDB[k].id == w.def->id) pool.push_back({ 0, k });
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

									if (type == 0) { // --- BRO— ---
										titleStr = weaponDB[idx].name;
										col = weaponDB[idx].color;
										for (auto& w : weaponInventory) {
											if (w.def->id == weaponDB[idx].id) {
												isUp = true;
												nextLvl = w.level + 1;
												break;
											}
										}
										int levelIndex = nextLvl - 1;
										if (levelIndex < (int)weaponDB[idx].levels.size()) descStr = weaponDB[idx].levels[levelIndex].description;
										else descStr = "Uzyskano maksymalny poziom";
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
						}

						expOrbs.erase(expOrbs.begin() + i);
					}
					else {
						i++;
					}
				}
				// 8. AKTUALIZACJA PORTALI
				for (auto& p : portals) {
					p.animTimer += dt;
					if (p.animTimer >= 0.15f) { // PrÍdkoúÊ animacji
						p.animTimer = 0.f;
						p.currentFrame = (p.currentFrame + 1) % 6; // Mamy 6 klatek

						// Przesuwamy okno wyboru klatki o 16 pikseli (szerokoúÊ jednej klatki)
						p.sprite.setTextureRect(sf::IntRect({ p.currentFrame * 32, 0 }, { 32, 32 }));
					}
				}
			}
			// 9. AKTUALIZACJA ANIMACJI PIORUN”W
			for (size_t i = 0; i < lightnings.size(); ) {
				lightnings[i].animTimer += dt;

				if (lightnings[i].animTimer >= 0.04f) { // szybkoúÊ animacji
					lightnings[i].animTimer = 0.f;
					lightnings[i].currentFrame++;
					if (lightnings[i].currentFrame < 13) {
						lightnings[i].sprite.setTextureRect(
							sf::IntRect({ lightnings[i].currentFrame * 64,0 }, { 64, 64 }));

					}

				}
				// --- USUWANIE PO ANIMACJI ---
				if (lightnings[i].currentFrame >= 13) {
					lightnings.erase(lightnings.begin() + i);
				}
				else {
					++i;
				}
			}
			// 10. --- AKTUALIZACJA ANIMACJI SZALIKA ---
			for (auto it = szalik.begin(); it != szalik.end(); ) {

				float finalDir = (float)(facingDir * it->side);

				// Pobieramy zapisanπ wielkoúÊ (to jest odpowiednik Twojego stats.area)
				float currentArea = it->scaleMult;

				// Obliczamy przesuniÍcie dynamicznie (matematyka zamiast if-Ûw)
				// Dla 1.0 -> 10 + 75 = 85.f
				// Dla 1.1 -> 10 + 82.5 = 92.5f
				// Dla 1.5 -> 10 + 112.5 = 122.5f
				float offset = 10.f + (75.f * currentArea);

				it->sprite.setPosition(player.getPosition() + sf::Vector2f(offset * finalDir, 0.f));



				// --- ZMIANA TUTAJ ---
				// Dodajemy "sztuczny" mnoønik wizualny (np. 1.8f, czyli powiÍkszenie o 80%)
				// scaleMult (wp≥yw Candla) nadal dzia≥a, ale ca≥oúÊ jest mnoøona razy 1.8
				float visualBoost = 1.4f;
				float currentScale = 0.125f * it->scaleMult * visualBoost;
				// --------------------

				// Aplikujemy skalÍ (z minusem dla odwrÛcenia)
				if (finalDir < 0) it->sprite.setScale({ -currentScale, currentScale });
				else it->sprite.setScale({ currentScale, currentScale });

				// C. Przewijanie klatek (bez zmian)
				it->animTimer += dt;
				if (it->animTimer >= 0.08f) {
					it->animTimer = 0.f;
					it->currentFrame++;

					if (it->currentFrame >= 5) {
						it->finished = true;
						it->currentFrame = 4;
					}
					else {
						it->sprite.setTextureRect(sf::IntRect({ it->currentFrame * 1024, 0 }, { 1024, 1024 }));
					}
				}

				if (it->finished) {
					it = szalik.erase(it);
				}
				else {
					++it;
				}
			}
			// --- Aktualizacja animacji wrogÛw ---

			for (size_t i = 0; i < Ochrona.size(); ) {
				auto& w = Ochrona[i];
				// 1. Znajdü w≥aúciciela (hitbox) po ID
				bool foundOwner = false;
				sf::Vector2f targetPos;

				for (const auto& en : enemies) {
					if (en.id == w.targetId) {
						targetPos = en.shape.getPosition();
						foundOwner = true;
						break;
					}
				}
				// Jeúli hitbox nie istnieje (wrÛg zginπ≥), usuÒ animacjÍ
				if (!foundOwner) {
					Ochrona.erase(Ochrona.begin() + i);
					continue;
				}

				// 2. Aktualizacja pozycji
				w.sprite.setPosition(targetPos);
				// 3. OBRACANIE W STRON  GRACZA (FLIP)
				// Jeúli gracz jest po lewej stronie wroga
				if (player.getPosition().x < targetPos.x) {
					w.sprite.setScale({ -2.f, 2.f }); // OdwrÛÊ w poziomie (minus na X)
				}
				else {
					w.sprite.setScale({ 2.f, 2.f });  // Normalna skala (patrzy w prawo)
				}
				// 4. ANIMACJA (ZWOLNIONA)
				w.animTimer += dt;
				// ZMIANA: ZwiÍkszono z 0.04f na 0.12f (im wiÍcej, tym wolniej)
				if (w.animTimer >= 0.12f) {
					w.animTimer = 0.f;
					w.currentFrame++;
					if (w.currentFrame >= 10) {
						w.currentFrame = 0;
					}

					w.sprite.setTextureRect(
						sf::IntRect({ w.currentFrame * 128, 0 }, { 128, 128 }));
				}
				i++;
			}

			for (size_t i = 0; i < Szczur.size(); ) {
				auto& w2 = Szczur[i];
				// 1. Znajdü w≥aúciciela (hitbox) po ID
				bool foundOwner = false;
				sf::Vector2f targetPos;
				for (const auto& en : enemies) {
					if (en.id == w2.targetId) {
						targetPos = en.shape.getPosition();
						foundOwner = true;
						break;
					}
				}
				// Jeúli hitbox nie istnieje (wrÛg zginπ≥), usuÒ animacjÍ
				if (!foundOwner) {
					Szczur.erase(Szczur.begin() + i);
					continue;
				}

				// 2. Aktualizacja pozycji
				w2.sprite.setPosition(targetPos);
				// 3. OBRACANIE W STRON  GRACZA (FLIP)
				// Jeúli gracz jest po lewej stronie wroga
				if (player.getPosition().x < targetPos.x) {
					w2.sprite.setScale({ -2.f, 2.f }); // OdwrÛÊ w poziomie (minus na X)
				}
				else {
					w2.sprite.setScale({ 2.0f, 2.0f });  // Normalna skala (patrzy w prawo)
				}
				// 4. ANIMACJA (ZWOLNIONA)
				w2.animTimer += dt;
				// ZMIANA: ZwiÍkszono z 0.04f na 0.12f (im wiÍcej, tym wolniej)
				if (w2.animTimer >= 0.12f) {
					w2.animTimer = 0.f;
					w2.currentFrame++;

					if (w2.currentFrame >= 5) {
						w2.currentFrame = 0;
					}

					w2.sprite.setTextureRect(
						sf::IntRect({ w2.currentFrame * 48, 0 }, { 48, 48 }));
				}
				i++;
			}

			// --- Aktualizacja animacji Egzaminu ---
			for (size_t i = 0; i < Egzamin.size(); ) {
				auto& w3 = Egzamin[i];

				// 1. Znajdü w≥aúciciela (hitbox) po ID
				bool foundOwner = false;
				sf::Vector2f targetPos;
				for (const auto& en : enemies) {
					if (en.id == w3.targetId) {
						targetPos = en.shape.getPosition();
						foundOwner = true;
						break;
					}
				}
				// Jeúli hitbox nie istnieje (wrÛg zginπ≥), usuÒ animacjÍ
				if (!foundOwner) {
					Egzamin.erase(Egzamin.begin() + i);
					continue;
				}

				// 2. Aktualizacja pozycji
				w3.sprite.setPosition(targetPos);

				// 3. OBRACANIE W STRON  GRACZA (FLIP)
				// Uøywamy skali 2.5f (takiej samej jak w konstruktorze)
				float scaleVal = 2.5f;

				if (player.getPosition().x < targetPos.x) {
					w3.sprite.setScale({ -scaleVal, scaleVal }); // OdwrÛÊ w poziomie
				}
				else {
					w3.sprite.setScale({ scaleVal, scaleVal });  // Normalna skala
				}

				// 4. PRZEWIJANIE KLATEK
				w3.animTimer += dt;
				if (w3.animTimer >= 0.12f) { // PrÍdkoúÊ animacji
					w3.animTimer = 0.f;
					w3.currentFrame++;

					if (w3.currentFrame >= 5) { // Masz 5 klatek (0,1,2,3,4)
						w3.currentFrame = 0;
					}

					// Przesuwamy okno o 38 pikseli
					w3.sprite.setTextureRect(
						sf::IntRect({ w3.currentFrame * 38, 0 }, { 38, 38 }));
				}
				i++;
			}

			// --- Aktualizacja animacji BOSSA (64x64) ---
			for (size_t i = 0; i < Boss.size(); ) {
				auto& b = Boss[i];

				bool foundOwner = false;
				sf::Vector2f targetPos;
				for (const auto& en : enemies) {
					if (en.id == b.targetId) {
						targetPos = en.shape.getPosition();
						foundOwner = true;
						break;
					}
				}

				// Jeúli Boss umar≥, usuÒ animacjÍ
				if (!foundOwner) {
					Boss.erase(Boss.begin() + i);
					continue;
				}

				// Aktualizacja pozycji
				b.sprite.setPosition(targetPos);

				// 3. OBRACANIE W STRON  GRACZA (FLIP)
				float scaleVal = 4.0f;

				if (player.getPosition().x < targetPos.x) {
					// Jeúli gracz jest po lewej:
					// USUN•£EM MINUS TUTAJ (by≥o -scaleVal, jest scaleVal)
					b.sprite.setScale({ scaleVal, scaleVal });
				}
				else {
					// Jeúli gracz jest po prawej:
					// DODA£EM MINUS TUTAJ (by≥o scaleVal, jest -scaleVal)
					b.sprite.setScale({ -scaleVal, scaleVal });
				}

				// Przewijanie klatek
				b.animTimer += dt;
				if (b.animTimer >= 0.15f) { // Boss moøe machaÊ skrzyd≥ami wolniej/dostojniej
					b.animTimer = 0.f;
					b.currentFrame++;

					if (b.currentFrame >= 5) { // 5 klatek
						b.currentFrame = 0;
					}

					// Przesuwamy okno o 64 piksele
					b.sprite.setTextureRect(
						sf::IntRect({ b.currentFrame * 64, 0 }, { 64, 64 }));
				}
				i++;
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

				//Czy blizez tej lini tym dalej jest rysowane
				for (auto& p : portals) window.draw(p.sprite);
				for (auto& z : zones) window.draw(z.shape);
				for (auto& l : lightnings) window.draw(l.sprite);
				for (auto& z : zones) {
					window.draw(z.shape);
				}
				for (auto& o : expOrbs) window.draw(o.sprite);
				for (auto& e : enemies) window.draw(e.shape);
				for (auto& p : projectiles) {
					if (p.wId == WHIP) window.draw(p.boxShape);
					else if (p.wId == LIGHTNING) continue;
					else window.draw(p.shape);
				}
				for (auto& w : Ochrona)
					window.draw(w.sprite);
				for (auto& w2 : Szczur)
					window.draw(w2.sprite);
				for (auto& w3 : Egzamin)
					window.draw(w3.sprite);
				for (auto& b : Boss)
					window.draw(b.sprite);
				for (auto& s : szalik) {
					window.draw(s.sprite);
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
				for (auto& w : weaponInventory) wList += w.def->name + " " + std::to_string(w.level) + "\n";
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
					window.draw(pauseStatsText);
					window.draw(pauseTitle);
					window.draw(resumeInfo);
					window.draw(pauseRestart);
					window.draw(pauseMainMenu);
				}
			}

			window.display();
		}

	}
	return 0;
}