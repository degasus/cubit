#ifndef _CONFIG_H_
#define _CONFIG_H_

class Config;

/**
 *
 *
 */
class Config {
public:
	/**
	 * Läd die Konfigurationsdateien
	 */
	Config(const char* filename);

	/**
	 * Gibt den Inhalt einer Konfig aus
	 */
	const char* operator[](const char* name);
};

#endif
