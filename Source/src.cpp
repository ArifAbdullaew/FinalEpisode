#include "src.h"

/**
 * набор уровней сложности, свой выдавать нельзя
 */
std::array<difficulty_t, 3> difficulties;

/**
 * менеджер текстур, это просто инициализация
 */
alone::TextureManager textures;

/**
 * то же самое, но для состояний
 */
alone::StateMachine states;


/**
 * базовая инициализация основных элементов
 */
sf::RenderWindow window(sf::VideoMode(450, 800), "Minesweeper");
sf::Font font;

LRMB lrmb;

/**
 * контейнер для управления текстурами
 */
void alone::TextureManager::load(std::string config_name) {
    /**
     * подгрузка текстур из конфига
     */
    std::ifstream file(config_name);

    while (!file.eof()) {
        std::string temp;
        file >> temp;

        sf::Texture texture;
        texture.loadFromFile("assets/textures/" + temp);
        /**
         * Вставка в контейнер
         */
        _Content.emplace(temp, std::move(texture));
    }
}

/**
 * перегрузка оператора индексирования
 */
sf::Texture &alone::TextureManager::operator[](std::string key) {
    return _Content.at(key);
}

/**
 * Вставка нового состояния с определённым ключом и заранее созданным состоянием
 */
void alone::StateMachine::insert(std::string key, std::shared_ptr<State> value) {
    value->_Status = State::OnCreate;
    _Content.emplace(key, value);
}

/**
 * Убирает состояние из контейнера, но не сразу же, а после следующего обновления
 */
void alone::StateMachine::erase(std::string key) {
    auto it = _Content.find(key);
    if (it != _Content.end())
        it->second->_Status = State::OnDelete;
}

void alone::StateMachine::update() {
    /**
     *  была проблема с контейнером, нельзя во время иттерации элементы удалять
		поэтому сделали очередь для этих элементов
     */
    std::queue<std::string> onRemove;
    for (auto &it: _Content) {
        switch (it.second->_Status) {
            case State::OnCreate:
                it.second->onCreate();
                it.second->_Status = State::Active;
                break;

                /**
                 * основной статус, в котором проводит время состояние игры
                 */
            case State::Active:
                it.second->update();
                it.second->draw(window, sf::RenderStates::Default);
                break;


            case State::OnDelete:
                it.second->onDelete();
                onRemove.push(it.first);
                break;
        }
    }
    /**
     * Непосредственное удаление элементов, которые были запрошены для этого во время работы процесса
     */
    while (!onRemove.empty()) {
        _Content.erase(onRemove.front());
        onRemove.pop();
    }
}

/**
 * работа с кнопками
 */
void alone::input::update() {
    lrmb.preLmb = lrmb.nowLmb;
    lrmb.preRmb = lrmb.nowRmb;
    lrmb.nowLmb = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    lrmb.nowRmb = sf::Mouse::isButtonPressed(sf::Mouse::Right);
}

/**
 * для левой кнопки мыши
 * @return выдаёт true только если кнопка была только что поднята
 */
bool alone::input::isClickedLeftButton() {
    return lrmb.preLmb && !lrmb.nowLmb;
}

/**
 * для правой
 */
bool alone::input::isClickedRightButton() {
    return lrmb.preRmb && !lrmb.nowRmb;
}

/**
 * Работа с картой
 */
void Map::resize(size_t level) {
    /**
     * собственно и взятие уровня сложности
     */
    auto &d = difficulties[level];

    /**
     * изменение ширины поля карты
     */
    _Content.resize(d.size);
    for (auto &it: _Content)
        /**
         * изменение высота поля, для каждой колонки в отдельности
         */
        it.resize(d.size, {'n', Type::None});
}


/**
 *  генерация карты, включая рандомное заполнение
	то же берёт заранее заготовленный уровень сложности из std::array <difficulty_t, 3> difficulties
	сделано, чтобы игрок не мог проиграть с первого нажатия
 *  @param point это точка, в которую нажал игрок
 */
void Map::generate(size_t level, sf::Vector2u point) {
    auto &d = difficulties[level];
    _Bombs = d.bombs;

    /**
     *  переменная для незаполненных клеток карты
	    достаточно много весит, вначале 'размер_грани ^ 2 - 1'
     */
    std::set<size_t> unfilled;

    /**
     *  заполняем все точки с карты так, что элемент с индексом 0 - это левый верхний
	    а с индексом 10 при ширине в 8 тайлов - это элемент с 'x = 1' и 'y = 2'
     */
    for (size_t i = 0; i != d.size * d.size; i++)
        unfilled.emplace(i);

    /**
     * убираем ту самую точку, в которую вначале тыкнул игрок
     */
    unfilled.erase(point.x + point.y * d.size);

    /**
     * рандомный генератор
     */
    std::random_device rd;

    /**
     * заполнение бомб
     */
    for (size_t i = 0; i != _Bombs; i++) {
        /**
         * генерация рандомного индекса числа из сета
         */
        std::uniform_int_distribution<size_t> dist(0, unfilled.size() - 1);
        size_t pos = dist(rd);
        //std::cout << pos << ' ';

        auto it = unfilled.begin();
        std::advance(it, pos);

        /**
         * point - это точка, в которую поместят бомбу
         */
        size_t point = *it;
        _Content[point % d.size][point / d.size] = {'n', Type::Bomb};
        /**
         * удаляет это точку из unfilled, тк она уже заполнена
         */
        unfilled.erase(point);
    }
    //std::cout << '\n';

    /**
     * заполнение чисел вокруг бомб
     */
    for (size_t i = 0; i != _Content.size(); i++) {
        for (size_t j = 0; j != _Content[i].size(); j++) {

            /**
             * пропускаем, если здесь есть бомба
             */
            if (_Content[i][j].second == Type::Bomb)
                continue;

            /**
             * проверяет соседние клетки от тайла
             */
            size_t value = _DetectAround(i, j);

            /**
             * изменяет значение кол-ва бомб, если их больше 0
             */
            if (value != 0)
                _Content[i][j].second = (Type) (value - 1);
        }
    }
}

/**
 * проверяет, есть ли бомба по заданному индексу
 * @return если выходит индекс за пределы карты, то возвращает false
 */
bool Map::_HasBomb(size_t x, size_t y) {
    if (x < 0 || x >= _Content.size() || y < 0 || y >= _Content.size())
        return false;
    return _Content[x][y].second == Type::Bomb;
};

/**
 * проверяет все клетки сверху, снизу, по бокам и по диагонали
 * @return
 */
size_t Map::_DetectAround(size_t x, size_t y) {
    return _HasBomb(x - 1, y - 1) + _HasBomb(x, y - 1) + _HasBomb(x + 1, y - 1) +
           _HasBomb(x - 1, y) + _HasBomb(x + 1, y) +
           _HasBomb(x - 1, y + 1) + _HasBomb(x, y + 1) + _HasBomb(x + 1, y + 1);
};

/**
 *  рекурсивный алгоритм, чтобы открыть тайлы вокруг нажатого
    получается рекурсия только в том случае, если игрок нажимает по пустому тайлу
 */
void Map::_OpenTiles(int x, int y) {
    if (y >= _Content.size() || y < 0 || x >= _Content.size() || x < 0)
        return;

    if (_Content[x][y].first == 'r' || _Content[x][y].second != Type::None) {

        /**
         * ставит статус revealed - проверено
         */
        _Content[x][y].first = 'r';
        return;
    }

    /**
     * ставит статус revealed - проверено
     */
    _Content[x][y].first = 'r';

    /**
     * проверка верхних
     */
    _OpenTiles(x - 1, y - 1);
    _OpenTiles(x, y - 1);
    _OpenTiles(x + 1, y - 1);

    /**
     * по бокам
     */
    _OpenTiles(x - 1, y);
    _OpenTiles(x + 1, y);

    /**
     * нижних
     */
    _OpenTiles(x - 1, y + 1);
    _OpenTiles(x, y + 1);
    _OpenTiles(x + 1, y + 1);
}

/**
 *  состояние для меню, чтобы было проще ей управлять
    может отключать игру и переводить в активное состояние
 */
void MenuState::update() {

    /**
     * получаем координаты мышки на экране в зависимости от положения самого окна
     */
    auto mouse = sf::Mouse::getPosition(window);

    /**
     * кнопочки для меню в отдельный массив, чтобы было проще их инициализировать
     */
    for (size_t i = 0; i != _Buttons.size(); i++) {

        /**
         * получаем глобальные координаты кнопочки
         */
        auto bounds = _Buttons[i].getGlobalBounds();
        //std::cout << mouse.x << ' ' << mouse.y << ' ' << alone::input::isClicked() << '\n';
        if (bounds.contains(mouse.x, mouse.y) && alone::input::isClickedLeftButton()) {
            _Params[i].second();
        }
    }
}

/**
 * создаёт интерфейс для меню и объявляет переменные
 */
void MenuState::onCreate() {

    /**
     * устанавливает размер экрана игры
     */
    window.setSize(sf::Vector2u(450, 800));

    /**
     * изменяет размер динамического массива с кнопками по количеству параметров
     */
    _Buttons.resize(_Params.size());

    for (size_t i = 0; i != _Buttons.size(); i++) {
        auto &text = _Buttons[i];

        /**
         * выставление дефолтных параметров
         */
        text.setString(_Params[i].first);
        text.setFont(font);
        text.setPosition(50, 100 + i * 50);

        auto bounds = text.getGlobalBounds();
        //std::cout << bounds.left << ' ' << bounds.top << ' ' << bounds.width << ' ' << bounds.height << '\n';
    }
}

/**
 * удалять тут нечего, так как не используем обычные указатели
 */
void MenuState::onDelete() {

}

/**
 * отрисовка кнопочек
 */
void MenuState::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    for (const auto &it: _Buttons)
        target.draw(it, states);
}
/**
* обновление экрана
*/
void GameOverState::update() {
    auto mouse = sf::Mouse::getPosition(window);
    auto bounds = _Exit.getGlobalBounds();

    /**
     * проверка, была ли нажата кнопка выхода из игры
     */
    if (bounds.contains(mouse.x, mouse.y) && alone::input::isClickedLeftButton()) {

        /**
         * убирает среди состояний саму себя
         */
        states.erase("over");

        /**
         * и добавляет состояние меню
         */
        states.insert("menu", std::shared_ptr<State>(new MenuState()));
    }
}

/**
 * Oбъявления интерфейса внутри кода
 */
void GameOverState::onCreate() {

    /**
     * объявление размера шрифта для надписи о статусе игры
     */
    _Label.setCharacterSize(42);

    /**
     * и кнопки выхода из неё
     */
    _Exit.setCharacterSize(42);

    /**
     * генерация текста, который увидит игрок, если выиграл или проиграл
     */
    std::string text;
    text = "You ";
    if (_Status)
        text += "win";
    else
        text += "lose";

    /**
     * текст для количества бомб, которые нашёл игрок
     */
    text += "\n\nYou found " + std::to_string(_BombsFound) + " bombs!";

    /**
     * установка шрифта
     */
    _Label.setFont(font);
    _Exit.setFont(font);

    /**
     * установка текста
     */
    _Label.setString(text);
    _Exit.setString("Exit");

    /**
     * цвет внутри текста
     */
    _Label.setFillColor(sf::Color::White);
    _Exit.setFillColor(sf::Color::White);

    /**
     * цвет обода текста
     */
    _Label.setOutlineColor(sf::Color::White);
    _Exit.setOutlineColor(sf::Color::White);

    /**
     * установка местоположения для главной надписи о статусе выигрыша игрока
     */
    _Label.setPosition(20, 20);

    /**
     * установка местоположения для кнопки выхода в главное меню в зависимости от предыдущей кнопки
     */
    auto labelBounds = _Label.getGlobalBounds();

    _Exit.setPosition(20, labelBounds.height + 40);

    auto exitBounds = _Exit.getGlobalBounds();

    /**
     * установка размера окна в зависимости от размера надписи о статусе выигрыша игрока
     */
    window.setSize(sf::Vector2u(labelBounds.width + 80, labelBounds.height + 80 + exitBounds.height));
}

/**
 * удалять нечего
 */
void GameOverState::onDelete() {

}

/**
 * быстрая отрисовка заранее заготовленных кнопочек
 */
void GameOverState::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    target.draw(_Label, states);
    target.draw(_Exit, states);
}

void GameState::update() {

    /**
     * размер грани карты
     */
    size_t edge_size = _GameMap->_Content.size();

    /**
     * меняем размер массива вершин для карты игры
	    умножаем на 4, тк квадратная карта
     */
    _RenderRegion.resize(4 * edge_size * edge_size);

    auto &map = _GameMap->_Content;

    /**
     * получения количества секунд после начала уровня
     */
    auto time = _Clock.getElapsedTime();
    size_t seconds = time.asSeconds();

    /**
     * Ставим текст с количеством прошедшего времени в минутах и секундах
     */
    _TimerLabel.setString(std::to_string(seconds / 60) + ':' + std::to_string(seconds % 60));

    /**
     * эффекты при нажатии на экран
     */
    auto mouse = sf::Mouse::getPosition(window);

    /**
     * Проверка на нажатие и его исход
     */
    bool contains = mouse.x >= 0 && mouse.x <= edge_size * 32 && mouse.y >= _InterfaceOffset &&
                    mouse.y <= edge_size * 32 + _InterfaceOffset;

    /**
     * если нажали, то проверяем, что там было
     */
    if (contains) {
        /**
         * точка, в которую попали мышкой
         */
        auto point = sf::Vector2u(mouse.x / 32, (mouse.y - _InterfaceOffset) / 32);

        /**
         * если левая кнопка мыши нажата
         */
        if (alone::input::isClickedLeftButton()) {

            /**
             * карта генерируется в момент первого нажатия на карту
             */
            if (_Revealed == 0) {

                /**
                 * генерация
                 */
                _GameMap->generate(_Level, point);

                /**
                 * устанавливаем текст с количеством оставшихся для поиска бомб
                 */
                _RemainedLabel.setString("Bombs remained: " + std::to_string(_GameMap->_Bombs));
                _Revealed++;

                /**
                 * если сгенерировалось в этой точке ничего, то ищем ближайшие пустые тайлы и с цифрами
                 */
                if (map[point.x][point.y].second == Type::None)

                    /**
                     * алгоритм рекурсивный
                     */
                    _GameMap->_OpenTiles(point.x, point.y);
                else

                    /**
                     * если статус отрисовки "неизвестный"
                     */
                    map[point.x][point.y].first = 'r';
            } else {
                if (map[point.x][point.y].first == 'n') {
                    if (map[point.x][point.y].second == Type::None)

                        /**
                         * то открываем рядом пустые тайлы до цифр
                         */
                        _GameMap->_OpenTiles(point.x, point.y);
                    else
                        /**
                         * установка статуса "видимый"
                         */
                        map[point.x][point.y].first = 'r';

                        /**
                         * если игрок нажал по бомбе левой кнопкой мыши, то он проиграл
                         */
                    if (map[point.x][point.y].second == Type::Bomb)
                        _GameStatus = 'l';

                    _Revealed++;
                }
            }

            /**
             * это проверка на нажатие левой кнопкой мыши
             */
        } else if (alone::input::isClickedRightButton()) {

            /**
             * если изначально статус тайла был флаг
             */
            if (map[point.x][point.y].first == 'f') {

                /**
                 * то меняем его на противоположный
                 */
                map[point.x][point.y].first = 'n';

                /**
                 * не забывая обновить счётчик бомб
                 */
                _RemainedLabel.setString("Bombs remained: " + std::to_string(++_GameMap->_Bombs));

                /**
                 * и уменьшая количество правильно расположенных на карте флагов
                 */
                if (map[point.x][point.y].second == Type::Bomb)
                    _Flags--;

                /**
                 * если же на тайле не было флага
                 */
            } else if (map[point.x][point.y].first == 'n') {

                /**
                 * то ставим его
                 */
                map[point.x][point.y].first = 'f';
                _RemainedLabel.setString("Bombs remained: " + std::to_string(--_GameMap->_Bombs));

                /**
                 * если всё верно, то инкрементируем счётчик правильных флагов, который не виден игроку
                 */
                if (map[point.x][point.y].second == Type::Bomb)
                    _Flags++;

                /**
                 * если все бомбы найдены, то игра заканчивается, а игрок выигрывает
                 */
                if (_Flags == difficulties[_Level].bombs) {
                    _GameStatus = 'w';
                }
            }
        }
    }

/**
 * расчёт вершин для карты
 */
    for (size_t i = 0; i != map.size(); i++) {
        for (size_t j = 0; j != map[i].size(); j++) {

            /**
             * это 4 вершины одного квадрата
             */
            auto &top_lhs = _RenderRegion[(i + j * map.size()) * 4];
            auto &top_rhs = _RenderRegion[(i + j * map.size()) * 4 + 1];
            auto &bot_rhs = _RenderRegion[(i + j * map.size()) * 4 + 2];
            auto &bot_lhs = _RenderRegion[(i + j * map.size()) * 4 + 3];

            /**
             * это id для отрисовки квадрата, показывает, какую точку у атласа с текстурами рисовать
             */
            size_t id = 0;

            /**
             * если тайл виден игроку, то
             */
            if (DEBUG_MODE || map[i][j].first == 'r')

                /**
                 * просто ставим то, что там есть
                 */
                id = (size_t) map[i][j].second;

                /**
                 * если тут флаш
                 */
            else if (map[i][j].first == 'f')
                /**
                 * то говорим рисовать флаг
                 */
                id = (size_t) Type::Flag;
            else
                /**
                 * иначе просто рисуем пустоту
                 */
                id = (size_t) Type::Unknown;


                /**
                 * это id с самой текстурами, так как текстура квадратная
                 */
            size_t idx = id % 4;
            size_t idy = id / 4;

            /**
             * расчёт местоположения на экране, включая смещение интерфейса по вертикали
             */
            top_lhs.position = sf::Vector2f(i * 32, _InterfaceOffset + j * 32);
            top_rhs.position = sf::Vector2f(i * 32 + 32, _InterfaceOffset + j * 32);
            bot_rhs.position = sf::Vector2f(i * 32 + 32, _InterfaceOffset + j * 32 + 32);
            bot_lhs.position = sf::Vector2f(i * 32, _InterfaceOffset + j * 32 + 32);

            /**
             * расчёт 4 вершин с текстуры, которые соответствуют реальной картинке с экрана
             */
            top_lhs.texCoords = sf::Vector2f(idx * 32.f, idy * 32.f);
            top_rhs.texCoords = sf::Vector2f(idx * 32.f + 32, idy * 32.f);
            bot_rhs.texCoords = sf::Vector2f(idx * 32.f + 32, idy * 32.f + 32);
            bot_lhs.texCoords = sf::Vector2f(idx * 32.f, idy * 32.f + 32);
        }
    }

    /**
     * проверка того, закончилась ли игра
     */
    if (_GameStatus != 'a') {
        states.erase("game");
        states.insert("over", std::shared_ptr<State>(new GameOverState(_GameStatus == 'w', _Flags)));
    }
}

void GameState::onCreate() {
    /**
     * обнуляем таймер, так как игра началась!
     */
    _Clock.restart();

    /**
     * сбрасываем карту игру и изменяем её размер в зависимости от уровня сложности
     */
    _GameMap.reset(new Map());
    _GameMap->resize(_Level);

    /**
     * количество открытых клеток с карты равно 0
     */
    _Revealed = 0;

    /**
     * размер ребра карты
     */
    size_t edge_size = _GameMap->_Content.size();

    /**
     * размер экрана игры зависит от размера самой карты
     */
    window.setSize(sf::Vector2u(edge_size * 32, edge_size * 32 + _InterfaceOffset));

    /**
     * атлас текстур
     */
    _Atlas = &textures["minesweeper.png"];

    /**
     * установка шрифта для надписей
     */
    _RemainedLabel.setFont(font);
    _TimerLabel.setFont(font);

    /**
     * цвет внутри
     */
    _RemainedLabel.setFillColor(sf::Color::White);
    _TimerLabel.setFillColor(sf::Color::White);

    /**
     * снаружи
     */
    _RemainedLabel.setOutlineColor(sf::Color::White);
    _TimerLabel.setOutlineColor(sf::Color::White);

    /**
     * координаты надписей
     */
    _RemainedLabel.setPosition(20, 15);
    _TimerLabel.setPosition(20, 50);

    /**
     * установка специального размера текста для самого лёгкого уровня сложности
     */
    if (_Level == 0) {
        _RemainedLabel.setCharacterSize(24);
        _TimerLabel.setCharacterSize(24);
    }
}

/**
 * тут же при удалении лучше перестраховаться и обнулить умный указатель
 */
void GameState::onDelete() {
    _GameMap.reset(nullptr);
}

/**
 * отрисовка самой игры
 */
void GameState::draw(sf::RenderTarget &target, sf::RenderStates states) const {
    states.texture = _Atlas;
    target.draw(_RenderRegion, states);

    target.draw(_RemainedLabel, states);
    target.draw(_TimerLabel, states);
}

MenuState::MenuState() {

    /**
     * заполняем поведение кнопок в случае нажатия для меню
     */
    _Params = {
            std::make_pair(std::string("Easy"), []() {
                states.insert("game", std::shared_ptr<alone::State>(new GameState(0)));
                states.erase("menu");
            }),
            std::make_pair(std::string("Medium"), []() {
                states.insert("game", std::shared_ptr<alone::State>(new GameState(1)));
                states.erase("menu");
            }),
            std::make_pair(std::string("Hard"), []() {
                states.insert("game", std::shared_ptr<alone::State>(new GameState(2)));
                states.erase("menu");
            }),
            std::make_pair(std::string("Exit"), []() {
                window.close();
            })
    };
}
