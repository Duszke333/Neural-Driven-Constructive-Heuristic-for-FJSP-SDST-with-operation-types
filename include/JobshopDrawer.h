#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "JobshopData.h"


namespace jobshop {
    using namespace std;

    /**
     * @brief Utility class for rendering Gantt charts of generated schedules using SFML.
     * * Creates visual representations of the assigned operations on machines
     * and exports them as image files (e.g., .png).
     */
    class JobshopDrawer {
    public:
        vector<sf::Color> Colors; ///< Palette of colors assigned to different jobs.
        sf::Font font; ///< Font used for rendering operation labels.

        /**
         * @brief Constructor initializing the font and color palette.
         */
        JobshopDrawer() {
            populateColors();

            if (!font.loadFromFile("fnt/Arimo-VariableFont_wght.ttf")) {
            }
        }

        /**
         * @brief Generates and shuffles a distinguishable color palette for jobs.
         */
        void populateColors();

        /**
         * @brief Renders the solution inside a JobshopData instance to an image file.
         * @param IOD The problem instance containing the solved schedule.
         * @param dir Directory where the image will be saved.
         * @param ext File extension/format (e.g., ".png").
         */
        void drawToFile(const JobshopData &IOD, string dir, string ext);
    };
}
