#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "JobshopData.h"


namespace jobshop {
    using namespace std;


    class JobshopDrawer {
    public:
        vector<sf::Color> Colors;
        sf::Font font;

        JobshopDrawer() {
            populateColors();

            if (!font.loadFromFile("fnt/Arimo-VariableFont_wght.ttf")) {
            }
        }

        void populateColors();

        void drawToFile(const JobshopData &IOD, string dir, string ext);

        //
        // void drawToFile(const PalOpt::InputOutputData &IOD, string ext) {
        //     float WIDTH = 700;
        //     float HEIGHT = 128;
        //     float SCALE = 4;
        //     float RES_SCALE = HEIGHT/min(IOD.PSizeX, IOD.PSizeY);
        //     sf::RenderTexture window;
        //     window.create(SCALE*(WIDTH+2), SCALE*(HEIGHT+2));
        //     sf::View view = window.getDefaultView();
        //     view.setSize(SCALE*(WIDTH+2), -SCALE*(HEIGHT+2));
        //     window.setView(view);
        //     window.clear(sf::Color::Black);
        //     sf::RectangleShape Background({SCALE*WIDTH, SCALE*HEIGHT});
        //     Background.setFillColor(sf::Color::White);
        //     Background.setPosition(SCALE*1, SCALE*1);
        //
        //     sf::Text text;
        //
        //     // select the font
        //     text.setFont(font); // font is a sf::Font
        //
        //     // set the string to display
        //     text.setString(to_string(IOD.obj));
        //
        //     // set the character size
        //     text.setCharacterSize(SCALE*10); // in pixels, not points!
        //
        //     text.setFillColor(sf::Color::Black);
        //     text.setPosition(SCALE*(WIDTH - 50), SCALE*2);
        //
        //
        //
        //     vector<sf::RectangleShape> Rects;
        //     for( auto &BP: IOD.Solution ) {
        //         auto &B = *BP.first;
        //         auto Pos = BP.second;
        //         Pos.X *= RES_SCALE;
        //         Pos.Y *= RES_SCALE;
        //
        //         int sx = B.SizeX * RES_SCALE;
        //         int sy = B.SizeY * RES_SCALE;
        //         if (Pos.Rotated) swap(sx, sy);
        //
        //         sf::RectangleShape Rect(sf::Vector2f(SCALE*(sx-2), SCALE*(sy-2)));
        //         if (B.idx < Colors.size()) {
        //             Rect.setFillColor(Colors[B.idx]);
        //             Rect.setOutlineColor(sf::Color::Black);
        //             Rect.setOutlineThickness(SCALE*1);
        //         }
        //         Rect.setPosition(sf::Vector2f(SCALE*(Pos.X+2), SCALE*(Pos.Y+2)));
        //         Rects.push_back(Rect);
        //     }
        //
        //     // while (window.isOpen()) {
        //     window.clear();
        //     window.draw(Background);
        //     for( auto &R: Rects) {
        //         window.draw(R);
        //     }
        //
        //     window.draw(text);
        //
        //     // }
        //     sf::Texture outputTexture = window.getTexture();
        //
        //     sf::Image output;
        //     output = outputTexture.copyToImage();
        //     string fn = IOD.fileName + "_" + ext;
        //     output.saveToFile(fn);
        //
        // }
    };
}


