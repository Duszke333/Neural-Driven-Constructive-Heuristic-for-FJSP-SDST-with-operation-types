#include <random>
#include <SFML/Graphics.hpp>
#include "JobshopDrawer.h"
#include "utils.h"
#include <set>
#include <string>

namespace jobshop {
    using namespace nnutils;

    void JobshopDrawer::populateColors() {
        Colors.clear();
        for (int r = 0; r < 4; r++) {
            for (int g = 0; g < 4; g++) {
                for (int b = 0; b < 4; b++) {
                    if (r == b && g == b && r == g) continue;
                    Colors.push_back(sf::Color(r * 64, g * 64, b * 64));
                }
            }
        }

        shuffle(Colors.begin(), Colors.end(), std::mt19937(1));
    }


    void JobshopDrawer::drawToFile(const JobshopData &IOD, string dir, string ext) {
        const float MACHINE_SIZE = 128;
        const float IMG_SIZE = MACHINE_SIZE * IOD.numM;

        const float TSCALE = 4.0f;

        float WIDTH = IMG_SIZE;
        float HEIGHT = IOD.Solution.obj * TSCALE + 60;


        sf::RenderTexture window;
        window.create(WIDTH + 4, HEIGHT + 4);
        sf::View view = window.getDefaultView();
        view.setSize(WIDTH + 4, -(HEIGHT + 4));
        window.setView(view);
        window.clear(sf::Color::Black);
        sf::RectangleShape Background({WIDTH, HEIGHT});
        Background.setFillColor(sf::Color::White);
        Background.setPosition(2, 2);

        sf::Text text;

        // select the font
        text.setFont(font); // font is a sf::Font

        // set the string to display
        text.setString(to_string_with_precision(IOD.getObj(), 0));

        // set the character size
        text.setCharacterSize(30); // in pixels, not points!
        text.setFillColor(sf::Color::Black);
        text.setPosition(WIDTH - MACHINE_SIZE * 3 / 4, IMG_SIZE / 100);

        vector<sf::RectangleShape> Rects;
        vector<string> Labels;

        sf::Text txtNum;

        // select the font
        txtNum.setFont(font); // font is a sf::Font
        txtNum.setCharacterSize(30); // in pixels, not points!
        txtNum.setFillColor(sf::Color::Black);

        int d = 0;
        for (auto &Dec: IOD.Solution.Decs) {
            int o = IOD.Jobs[Dec.j].Ops[Dec.iop];

            float PosX = Dec.m * MACHINE_SIZE + 2;
            float PosY = HEIGHT - (Dec.start_t * TSCALE - 2);

            float sx = MACHINE_SIZE;
            float sy = (Dec.end_t - Dec.start_t) * TSCALE; //.OMtime[o][Dec.m] * TSCALE;

            sy = -sy;

            sf::RectangleShape Rect(sf::Vector2f(sx, sy));

            if (Dec.j < Colors.size()) {
                Rect.setFillColor(Colors[Dec.j]);
                Rect.setOutlineColor(sf::Color::Black);
                Rect.setOutlineThickness(-MACHINE_SIZE / 50);
            }
            Rect.setPosition(sf::Vector2f(PosX, PosY));
            Rects.push_back(Rect);
            Labels.push_back(to_string(Dec.j) + " (" + to_string(o) + ")");
            d += 1;
        }

        // while (window.isOpen()) {
        window.clear();
        window.draw(Background);

        for (int i = 0; i < Rects.size(); i++) {
            auto &R = Rects[i];
            window.draw(R);

            txtNum.setString(Labels[i]);

            auto P = R.getPosition();
            auto S = R.getSize();
            S /= 2.0f;
            auto NP = R.getPosition() + sf::Vector2f(5, -40); //R.getSize()/5.0f;
            txtNum.setPosition(NP);

            window.draw(txtNum);
        }

        window.draw(text);

        // }
        sf::Texture outputTexture = window.getTexture();

        sf::Image output;
        output = outputTexture.copyToImage();
        string fn = dir + "/" + IOD.name + ext;
        output.saveToFile(fn);
    }
}