#include "Source/src.cpp"

using namespace sf;



void init() {
    textures.load("assets/textures/include.txt");

    /**
     * заполнением параметров уровня сложности
     */
    difficulties[0] = {"Easy", 10, 8};
    difficulties[1] = {"Medium", 20, 10};
    difficulties[2] = {"Hard", 70, 20};

    /**
     * погружаем шрифт
     */
    font.loadFromFile("assets/font.ttf");

    /**
     * добавляем меню как активное состояние игры
     */
    states.insert("menu", std::shared_ptr<alone::State>(new MenuState()));
}

struct LRM {
    /**
     * это для левой кнопки мыши
     */
    bool preLmb = false, nowLmb = false;

    /**
     * это для правой
     */
    bool preRmb = false, nowRmb = false;

    /**
     *  pre - состояние нажатия во время прошлого обновления
	    now - во время текущего

	    обновляет текущее состояние нажатия обеих клавиш
     */

    void update() {
        preLmb = nowLmb;
        preRmb = nowRmb;
        nowLmb = sf::Mouse::isButtonPressed(sf::Mouse::Left);
        nowRmb = sf::Mouse::isButtonPressed(sf::Mouse::Right);
    }

    /**
     *  запрос состояния нажатия левой кнопки мыши
	    выдаёт true только если кнопка была только что поднята
     */
    bool isClickedLeftButton() {
        return preLmb && !nowLmb;
    }

    /**
     * то же самое, но для правой
     */
    bool isClickedRightButton() {
        return preRmb && !nowRmb;
    }
};

/**
 * упрощение нейминга
 */
template<class _T>
using Matrix = std::vector<std::vector<_T>>;



int main() {
    init();

    /**
    * Музыкальное сопровождение
    */
    Music music;

    music.openFromFile("audio/mysaca.ogg");
    music.setVolume(50);

    music.play();
    while (window.isOpen()) {

        /**
         * это проверка ивентов самого окна
         */
        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {

                /**
                 * если окно было закрыто
                 */
                case sf::Event::Closed:
                    window.close();
                    break;

                    /**
                     * если окну поменяли размер
                     */
                case sf::Event::Resized:
                    window.setView(sf::View(sf::FloatRect(0, 0, event.size.width, event.size.height)));
                    break;
            }
        }

        /**
         * очищаем окно
         */
        window.clear();

        /**
         * обновляем ввод
         */
        alone::input::update();

        /**
         * а затем все состояния
         */
        states.update();

        /**
         * выводим на экран
         */
        window.display();
    }

    return 0;
}