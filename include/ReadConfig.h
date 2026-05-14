/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Params2.h
 * Author: tsliwins
 *
 * Created on 11 marca 2021, 18:34
 */

#pragma once

#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <type_traits>
#include <regex>


/**
 * Biblioteka na podstawie pliku konfiguracyjnego i parametrow linii polecen
 * tworzy mape typu
 *
 * typedef map<string, map<string, string>> ConfigMap;
 *
 * Mapa ma podwójny klucz: [sekcja][klucz] = wartosc
 *
 * 'sekcja' moze być stringiem pustym.
 *
 * Mapa przechwuje wszystkie wczytane klucze w postaci string, dopiero przy odczycie
 * są one konwertowane do postaci docelowej
 *
 * Plik konfiguracyjny jest typu .ini
 * zawiera linie z nazwą sekcj:
 *
 * [nazwa sekcji]
 *
 * oraz klucz wartość
 *
 * klucz=wartosc
 *
 * Moze wystapic klucz=wartosc przed nazwa sekcji, wtedy sekcja klucza jest stringiem pustym.
 *
 * Wartosc moze byc liczbą lub stringiem, a nawet stringiem oznaczajacym enum
 * Wartosc moze byc jednowymiarowym wektorem rozdzielonym spacjami, w jednej linii
 *
 * Komentarze sa od srednika ; do konca linii
 *
 * W linii poleceń występują wyłącznie argumenty w jednej z postaci:
 * --sekcja.klucz=wartosc
 * --klucz=wartosc
 *
 * W drugim przypadku sekcja jest stringiem pustym
 * Wartość również może być wektorem rozdzielonym spacjami
 * *******************************************************
 * Odczyt parametrow z mapy obywa sie za pomoca funkcji
 *
 * read
 * get
 *
 * oraz makr ułatwiających odczyt. Makra umożliwiają nie podawanie nazwy sekcji.
 * W takim przypadku nazwa sekcji jest podana w zmiennej section_name w tej samej
 * przestrzeni, w której jest wolane makro.
 *
 * READ -- odczyt bez wartosci domyslnej
 * READD -- odczyt z wartoscia domyslna w sytuacji braku klucza
 * GET
 * GETD
 *
 * READ(var)             - var - zmienna, parametr ma taką samą nazwę jak zmienna
   READD(var, def)       - def - dodatkowa wartość domyślna
   GET(typ, kn)           - typ, kn - string będący szukanym kluczem,
   GETD(typ, kn, def)     - def - dodatkowa wartosc domyslna
 *
 *
 * Klasa Enum jest pomocnicza i ulatwia budowanie obiektów typu wyliczanego
 * które się ładnie deserializują ze stringów. Poniżej przykład takiej klasy.
 * Obiekty wyliczane dla tego przykładu należy definiować np. tak:
 * PsiStrategy ps;
 *
 *
 struct _PsiStrategy {

    enum enum_type {
        noNPI = 0, evenly, greedy, deathRatio, file
    };

    static const vector<string> names() {
        return {"noNPI", "evenly", "greedy", "deathRatio", "file"};
    }
};
typedef Enum<_PsiStrategy> PsiStrategy;
 *
 *
 */


namespace ReadConfig {
    using namespace std;

    struct ConfigErr {
        string msg;

        ConfigErr(string _msg) : msg(_msg) {
        }
    };

    struct ConfigMap {
        int analyseCommandLine(int argc, const char **argv) {
            cmatch sm;

            // polaczenie linii polecen w jeden string
            string cmd;

            for (int i = 1; i < argc; i++) {
                cmd += argv[i];
            }

            // podzial na kawalki rozdzielone '--'
            vector<string> lines = split(cmd, "--");


            // analiza wyniku
            regex sec_key_value(R"rgx(\s*(([^\s=\.]*)\.)*([^\s=]*)\s*=\s*([^\r\n;]+?)\s*(;.*)?[$\r\n]*)rgx");

            for (auto &line: lines) {
                if (line.empty()) continue;
                bool err = false;
                if (regex_match(line.c_str(), sm, sec_key_value)) {
                    if (sm.size() > 4) {
                        cm[sm[2]][sm[3]] = sm[4];
                    } else {
                        err = true;
                    }
                } else {
                    err = true;
                }
                if (err) {
                    string msg = string("Unrecognized command line option: '") + line + "'";
                    cerr << msg << endl;
                    throw ConfigErr(msg);
                }
            }

            return 0;
        }

        inline void analyseConfigFile(string config_file) {
            cmatch sm;

            if (!config_file.empty()) {
                ifstream ifs(config_file);

                string last_section;
                string line;

                regex section(R"rgx([^\[]*\[\s*([^\s\]]+)\s*\]\s*([;#].*)?[$\r\n]*)rgx");
                regex key_value(R"rgx(\s*([^\s=]+)\s*=\s*([^\r\n;#]+?)\s*([;#].*)?[$\r\n]*)rgx");

                regex empty_line(R"rgx(\s*([;#].*)?[$\r\n]*)rgx");

                int ln = 1;
                while (getline(ifs, line)) {
                    bool err = false;
                    if (regex_match(line.c_str(), sm, section)) {
                        if (sm.size() > 1) {
                            last_section = sm[1];
                        } else {
                            err = true;
                        }
                    } else if (regex_match(line.c_str(), sm, key_value)) {
                        if (sm.size() > 2) {
                            cm[last_section][sm[1]] = sm[2];
                        } else {
                            err = true;
                        }
                    } else if (regex_match(line.c_str(), sm, empty_line)) {
                        // pusta linia albo sam komentarza
                    } else {
                        err = true;
                    }
                    if (err) {
                        string msg = string("Config file error, line ") + to_string(ln) + ": '" + line + "'";
                        cerr << msg << endl;
                        throw ConfigErr(msg);
                    }
                    ln += 1;
                }
            }
        }

    private:
        inline string find_val(string sec, string key) const {
            string s;
            auto it = cm.find(sec);
            if (it != cm.end()) {
                auto itkey = it->second.find(key);
                if (itkey != it->second.end()) {
                    s = itkey->second;
                }
            }
            return s;
        }

        template<class T>
        struct type {
        };

        template<class T>
        T get(const string &sec, const string &key, type<T>, int n = -1) const {
            string s = find_val(sec, key);

            if (s.empty()) {
                string msg = string("no value for ") + sec + "." + key;
                cerr << msg << endl;
                throw msg;
            }
            return decodeVector<T>(sec, key, s)[0];
        }

        template<class T>
        vector<T> get(const string &sec, const string &key, type<vector<T> >, int n = -1) const {
            string s = find_val(sec, key);

            if (s.empty()) {
                string msg = string("no value for ") + sec + "." + key;
                cerr << msg << endl;
                throw msg;
            }

            return decodeVector<T>(sec, key, s, type<T>{}, n);
        }

    public:
        template<class T>
        T get(const string &sec, const string &key) const {
            return get(sec, key, type<T>{});
        }

        template<class T>
        T get(const string &sec, const string &key, const T &def) const {
            string s = find_val(sec, key);
            if (s.empty()) {
                return def;
            } else {
                return decodeVector<T>(sec, key, s)[0];
            }
        }

        template<class T>
        vector<T> get(const string &sec, const string &key, const vector<T> &def, int n = -1) const {
            string s = find_val(sec, key);
            if (s.empty()) {
                return def;
            } else {
                return decodeVector<T>(sec, key, s, n);
            }
        }

        template<class T>
        void read(const string &sec, const string &key, T &val) const {
            val = get<T>(sec, key);
        }

        template<class T>
        void read(const string &sec, const string &key, const T &def, T &val) const {
            val = get<T>(sec, key, def);
        }

    private:
        template<class T>
        static vector<T> decodeVector(const string &sec, const string &key, const string &s, type<T>, int n = -1) {
            vector<T> vec;
            std::istringstream ss(s);
            T d;
            while (ss >> d) {
                vec.push_back(d);
            }
            if (!ss.eof() && ss.fail() || vec.empty()) {
                string msg = string("bad value for ") + sec + "." + key;
                cerr << msg << endl;
                throw ConfigErr(msg);
            }
            if (n > 0 && vec.size() != n) {
                string msg = string("wrong vector size for ") + sec + "." + key;
                cerr << msg << endl;
                throw ConfigErr(msg);
            }
            return vec;
        }

        static vector<bool> decodeVector(const string &sec, const string &key, const string &s, type<bool>,
                                         int n = -1) {
            vector<bool> vec;
            std::istringstream ss(s);
            string d;
            while (ss >> d) {
                if ((d == "true") || (d == "yes") || (d == "1") || d == "on") {
                    vec.push_back(true);
                } else if (d == "false" || d == "no" || d == "0" || d == "off") {
                    vec.push_back(false);
                } else {
                    string msg = string("bad value '") + d + "' for " + sec + "." + key;
                    cerr << msg << endl;
                    throw msg;
                }
            }
            if (!ss.eof() && ss.fail() || vec.empty()) {
                string msg = string("bad value for ") + sec + "." + key;
                cerr << msg << endl;
                throw ConfigErr(msg);
            }
            if (n > 0 && vec.size() != n) {
                string msg = string("wrong vector size for ") + sec + "." + key;
                cerr << msg << endl;
                throw ConfigErr(msg);
            }
            return vec;
        }

        template<class T>
        static vector<T> decodeVector(const string &sec, const string &key, const string &s) {
            return decodeVector(sec, key, s, type<T>{});
        }

        static inline vector<std::string> split(const std::string str, const std::string regex_str) {
            std::regex regexz(regex_str);
            std::vector<std::string> list(std::sregex_token_iterator(str.begin(), str.end(), regexz, -1),
                                          std::sregex_token_iterator());
            return list;
        }

        map<string, map<string, string> > cm;
    };


    template<class E, class = std::enable_if_t<std::is_enum<E>
        {
        }

    > >
    inline static std::istream &operator>>(std::istream &in, E &e) {
        std::string token;
        in >> token;
        int i;
        vector<string> nam = enum_names(e);
        for (i = 0; i < nam.size(); i++) {
            if (nam[i] == token) {
                e = static_cast<E>(i);
                break;
            }
        }
        if (i == nam.size()) {
            in.setstate(std::ios_base::failbit);
        }
        return in;
    }


    template<class E>
    string enum_name(E e) {
        return enum_names(E())[static_cast<int>(e)];
    }

#define READ(var)                   read(section_name, #var, var)
#define READN(var, n)                   read(section_name, #var, var, n)
#define READD(var, def)             read(section_name, #var, def, var);
#define READDN(var, def, n)             read(section_name, #var, def, var n);
#define GET(typ, kn)                    get<typ>(section_name, kn);
#define GETN(typ, kn, n)                    get<typ>(section_name, kn, n);
#define GETD(typ, kn, def)               get<typ>(section_name, kn, def);
#define GETDN(typ, kn, def, n)              get<typ>(section_name, kn, def, n);
};


