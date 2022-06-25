#pragma once
//std
#include <unordered_map>
#include <set>
#include <string>
#include <fstream>
#include <vector>
#include <array>
#include <random>
#include <queue>
#include <functional>
#include <iostream>
#include <memory>

//sfml
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#define DEBUG_MODE 0


namespace alone {
    /**
     * контейнер для управления текстурами
     */
    class TextureManager {
    public:

        /**
         * подгрузка текстур из конфига
         */
        void load(std::string config_name);

        /**
         * перегрузка оператора индексирования для более красивого кода
         */
        sf::Texture &operator[](std::string key);

    private:
        /**
         * unordered_map работает быстрее обычной map
         */
        std::unordered_map<std::string, sf::Texture> _Content;
    };

    /**
     * базовое состояние игры, от него наследуются все остальные
     * отвечает за всё в своём процессе
     * наследуется от Drawable, чтобы было меньше кода писать
     */
    class State : public sf::Drawable {
        friend class StateMachine;

    public:

        /**
         * статусы для более удобного использования состояний игры
         */
        enum Status {
            OnCreate,
            Active,
            OnDelete
        };

    protected:
        /**
         * не стали использовать делту, ибо это бесполезно в сапёре
         */
        virtual void update() = 0;

        /**
         * базовый метод, которое вызывают при создании состояния
         */
        virtual void onCreate() = 0;

        /**
         * а этот при удалении
         */
        virtual void onDelete() = 0;

    private:
        Status _Status;
    };

    /**
     *  Машина состояний, являющаяся контейнром состояний и их инвокером
	    Также отвечает за отрисовку
     */
    class StateMachine {
    public:
        /**
         * Вставка нового состояния с определённым ключом и заранее созданным состоянием
         */
        void insert(std::string key, std::shared_ptr<State> value);

        /**
         * Убирает состояние из контейнера, но не сразу же, а после следующего обновления
         */
        void erase(std::string key);

        void update();

    private:
        std::unordered_map<std::string, std::shared_ptr<State>> _Content;
    };
}

struct LRMB {
    bool preLmb = false, nowLmb = false;
    bool preRmb = false, nowRmb = false;
};

namespace alone::input {

    void update();

    bool isClickedLeftButton();

    bool isClickedRightButton();
}

//just a crutch for fast naming
template<class _T>
using Matrix = std::vector<std::vector<_T>>;

/**
 * для удобной нумерации текстур
 */
enum class Type {
    Number1 = 0,
    Number2 = 1,
    Number3,
    Number4,
    Number5,
    Number6,
    Number7,
    Number8,
    None,
    Unknown,
    Flag,
    NoBomb,
    NoneQuiestion,
    UnknownQuestion,
    Bomb,
    RedBomb
};

/**
 * класс для удобного хранения уровня сложности
 */
struct difficulty_t {

    /**
     * имя уровня сложности
     */
    std::string name;

    /**
     * количество бомб на карте
     */
    size_t bombs;

    /**
     * размер грани карты, карта может быть только квадратная
     */
    size_t size;
};

/**
 * класс карты игры
 */
class Map {
    /**
     * позволяет работать с картой игры не напрямую
     */
    friend class GameState;

public:

    /**
     *  изменение размера карты игры в зависимости от уровня сложности
     */

    void resize(size_t level);

    /**
     *  генерация карты, включая рандомное заполнение
	    то же берёт заранее заготовленный уровень сложности из std::array <difficulty_t, 3> difficulties
	    сделано, чтобы игрок не мог проиграть с первого нажатия
     *  @param point точка, в которую нажал игрок
     */
    void generate(size_t level, sf::Vector2u point);

    /**
     * костыль из использования char'а как состояния для отрисовки
     */
    //n - unknown, r - revealed, f - flag
    Matrix<std::pair<char, Type>> _Content;

    /**
     * кол-во бомб на карте
     */
    size_t _Bombs;

    /**
     * проверяет, есть ли бомба по заданному индексу
     * @return если выходит индекс за пределы карты, то возвращает false
     */
    bool _HasBomb(size_t x, size_t y);

    /**
     * проверяет все клетки сверху, снизу, побокам и по диагонали
     */
    size_t _DetectAround(size_t x, size_t y);

    /**
     *  рекурсивный алгоритм, чтобы открыть тайлы вокруг нажатого
        получается рекурсия только в том случае, если игрок нажимает по пустому тайлу
     */
    void _OpenTiles(int x, int y);
};

/**
 *  состояние для меню, чтобы было проще ей управлять
    может отключать игру и переводить в активное состояние
 */
class MenuState : public alone::State {

public:
    MenuState();

private:
    /**
     * обновления каждый кадр
     */
    void update() override;

    /**
     * создаёт интерфейс для меню и объявляет переменные
     */
    void onCreate() override;

    void onDelete() override;

    void draw(sf::RenderTarget &target, sf::RenderStates states = sf::RenderStates::Default) const override;

private:

    /**
     * массив с кнопками, инициализируется только во время вызова метода onCreate
     */
    std::vector<sf::Text> _Buttons;

    /**
     * заранее заготовленные параметры для кнопок, создаются в конструкторе
     */
    std::array<std::pair<std::string, std::function<void()>>, 4> _Params;
};

/**
 *  состояние для окончания игры
    возникает, когда выигрываешь или проигрываешь
 */
class GameOverState : public alone::State {
public:
    /**
     * @param status - это состояние выигрыша
     * @param bombsFound - это количество бомб, которыен нашёл игрок
     */
    GameOverState(bool status, size_t bombsFound) {
        _Status = status;
        _BombsFound = bombsFound;
    }

    sf::Text _Label, _Exit;
    //1 = win, 0 = lose
    bool _Status;
    size_t _BombsFound;

    /**
     * обновление экрана
     */
    void update() override;

    /**
     * там тоже объявление интерфейса внутри кода
     */
    void onCreate() override;

    /**
     * удалять нечего
     */
    void onDelete() override;

    /**
     * быстрая отрисовка заранее заготовленных кнопочек
     */
    void draw(sf::RenderTarget &target, sf::RenderStates states = sf::RenderStates::Default) const override;
};

/**
 * главное состояние, состояние игры
 */
class GameState : public alone::State {
public:

    /**
     * установка уровня сложности
     */
    GameState(size_t level) {
        _Level = level;
    }

    /**
     * указатель на карту игры
     */
    std::unique_ptr<Map> _GameMap;

    /**
     * просто константа
     */
    const size_t _InterfaceOffset = 100;

    /**
     * количество заполненных флагов на карте
     */
    size_t _Flags = 0;

    /**
     * вершины для отрисовки карты
     */
    sf::VertexArray _RenderRegion = sf::VertexArray(sf::Quads);

    /**
     * атлас с текстурами для быстрой и правильной отрисовки вершин у карты
     */
    sf::Texture *_Atlas = nullptr;

    /**
     * уровень сложности
     */
    size_t _Level;

    /**
     * количество открытых клеток
     */
    size_t _Revealed = 0;

    /**
     * таймер игры
     */
    sf::Clock _Clock;

    /**
     * две надписи с прошедшим временем после начала игры и количеством оставшихся бомб
     */
    sf::Text _RemainedLabel, _TimerLabel;
    //a - active, w - win, l - lose
    char _GameStatus = 'a';

    void update() override;

    void onCreate() override;

    void onDelete() override;

    void draw(sf::RenderTarget &target, sf::RenderStates states = sf::RenderStates::Default) const override;
};

