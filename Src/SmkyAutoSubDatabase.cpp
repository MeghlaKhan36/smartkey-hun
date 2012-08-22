/* @@@LICENSE
*
*      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wctype.h>
#include <algorithm>
#include "SmkyAutoSubDatabase.h"
#include "SmkyUserDatabase.h"
#include "Settings.h"
#include "SmartKeyService.h"
#include "StringUtils.h"

#if 0 // Debug settings
#define DEBUG_STUB g_debug
#else
#define DEBUG_STUB(fmt, args...) (void)0
#endif

namespace SmartKey
{

// 10KB = approx 100 entries.
const size_t k_DatabaseSize = 30 * 1024;

SmkyAutoSubDatabase::SmkyAutoSubDatabase(SMKY_LINFO& lingInfo, const LocaleSettings& localeSettings, const Settings& settings) :
	SmkyDatabase(lingInfo)
	, m_settings(settings)
	, m_pDatabaseData(NULL)
	, m_createdDatabase(false)
	, m_locale(localeSettings)
{
}

SmkyAutoSubDatabase::~SmkyAutoSubDatabase()
{
	SMKY_STATUS wStatus = saveDb();
	if (wStatus) {
		g_warning("ERROR %d saving user db.", wStatus);
	}

	if (NULL != m_pDatabaseData) free(m_pDatabaseData);
}

bool SmkyAutoSubDatabase::didCreateDatabase() const
{
	return m_createdDatabase;
}

std::string SmkyAutoSubDatabase::getDbPath() const
{
	return m_settings.readWriteDataDir + "/smky/auto_sub_db_" + m_locale.getLanguageCountryLocale() + ".bin";
}


bool SmkyAutoSubDatabase::isWordAllUppercase(const uint16_t* word, uint16_t wordLen)
{
	if (word == NULL)
		return false;

	for (uint16_t i = 0; i < wordLen; i++) {
		if (!iswupper(word[i]))
			return false;
	}

	return true;
}

/**
 * Try to match the case of the guess to the input word. This way "cant" becomes "can't" and
 * "Cant" becomes "Can't".
 */
void SmkyAutoSubDatabase::matchGuessCaseToInputWord(const uint16_t* inputWord, uint16_t inputLen, uint16_t* guess, uint16_t guessLen)
{
	if (inputWord != NULL && guess != NULL && inputWord > 0 && iswupper(inputWord[0])) {

		// Default algorithm is to uppercase the first character of the guess if the first character 
		// of the input is uppercase.
		uint16_t numChars = 1;

		if (inputLen > 1 && isWordAllUppercase(inputWord, inputLen)) {
			// But if the input word is all uppercase then we will uppercase the entire guess.
			numChars = guessLen;
		}

		for (uint16_t i = 0; i < numChars; i++) {
			guess[i] = towupper(guess[i]);
		}
	}
}

SMKY_STATUS SmkyAutoSubDatabase::findEntry(const gunichar2* shortcut, uint16_t shortcutLen, std::string& substitution)
{
    return SMKY_STATUS_NONE;
}

std::string SmkyAutoSubDatabase::findEntry(const std::string& shortcut)
{
	return "";
}

SmartKeyErrorCode SmkyAutoSubDatabase::addEntry(const Entry& entry)
{
    return SKERR_SUCCESS;
}

SMKY_STATUS SmkyAutoSubDatabase::addEntry(const std::string& shortcut, const std::string& substitution)
{
    return SMKY_STATUS_NONE;
}

/**
 * Loads <b>all</b> user entries into the provided list - unsorted.
 */
SMKY_STATUS SmkyAutoSubDatabase::loadEntries(WhichEntries which, std::list<Entry>& entries) const
{
    entries.clear();

    return SMKY_STATUS_NONE;
}

static bool compare_entries (const SmkyAutoSubDatabase::Entry& first, const SmkyAutoSubDatabase::Entry& second)
{
	return StringUtils::compareStrings(first.shortcut, second.shortcut);
}

/**
 * Get all user entries
 */
SmartKeyErrorCode SmkyAutoSubDatabase::getEntries(int offset, int limit, WhichEntries which, std::list<Entry>& entries)
{
	if (offset < 0 || limit < 0)
		return SKERR_BAD_PARAM;

	entries.clear();

	std::list<Entry> allEntries;
	SMKY_STATUS wStatus = loadEntries(which, allEntries);
	if (wStatus == SMKY_STATUS_NONE) {
		allEntries.sort(compare_entries);
		std::list<Entry>::const_iterator i = allEntries.begin();
		// Skip up to the offset
		while (i != allEntries.end() && offset-- > 0) {
			++i;
		}

		// Now read the entries
		while (i != allEntries.end() && limit-- > 0) {
			entries.push_back(*i++);
		}
	}

	return SmkyUserDatabase::smkyErrorToSmartKeyError(wStatus);
}

/**
 * Return the number of user entries
 */
SmartKeyErrorCode SmkyAutoSubDatabase::getNumEntries(WhichEntries which, int& entries)
{
	entries = 0;

    return SmkyUserDatabase::smkyErrorToSmartKeyError(SMKY_STATUS_NONE);
}

SMKY_STATUS SmkyAutoSubDatabase::duplicateShortEntries()
{
    return SMKY_STATUS_NONE;
}

SMKY_STATUS SmkyAutoSubDatabase::loadDefaultData()
{
	SMKY_STATUS wStatus = SMKY_STATUS_NONE;

	int count = 0;
	std::string fname =  m_locale.findLocalResource(m_settings.readOnlyDataDir + "/smky/DefaultData/autoreplace/", "/text-edit-autoreplace");
	FILE* f = fopen(fname.c_str(),"rb");
	if (f) {
		char linebuffer[128];
		while (fgets(linebuffer, sizeof(linebuffer), f)) {
			char* shortcut = strtok(linebuffer,"|\x0d\x0a" );
			char* replacement = strtok(0,"\x0d\x0a" );
			if (shortcut != NULL && replacement != NULL && strlen(shortcut) > 0 && strlen(replacement) > 0) {
				SMKY_STATUS s = addEntry(shortcut, replacement);
				if (wStatus == SMKY_STATUS_NONE)
					wStatus = s, ++count;
			}
		}
	}
	else {
		wStatus = SMKY_STATUS_READ_DB_FAIL;
	}

	if (wStatus == SMKY_STATUS_NONE) {
		wStatus = duplicateShortEntries();
	}
	
	g_debug("Loaded %d auto-replace entries for locale '%s' from '%s'. Result: %u", count, m_locale.getFullLocale().c_str(), fname.c_str(), wStatus);

	return wStatus;
}

SMKY_STATUS SmkyAutoSubDatabase::loadHardCodedEntries()
{
	std::string fname =  m_locale.findLocalResource(m_settings.readOnlyDataDir + "/smky/DefaultData/autoreplace-hc/", "/text-edit-autoreplace");

	SMKY_STATUS wStatus;
	FILE* f = fopen(fname.c_str(),"rb");
	if (f) {
		wStatus = SMKY_STATUS_NONE;
		char linebuffer[128];
		while (fgets(linebuffer, sizeof(linebuffer), f)) {
			const char* shortcut = strtok(linebuffer,"|\x0d\x0a" );
			const char* replacement = strtok(0,"\x0d\x0a" );
			if (shortcut != NULL && replacement != NULL && strlen(shortcut) > 0 && strlen(replacement) > 0) {
				m_hardCodedEntries[shortcut] = replacement;
			}
		}

		g_debug("Loaded %u hard-code AR entries from '%s'", m_hardCodedEntries.size(), fname.c_str());
	}
	else {
		wStatus = SMKY_STATUS_READ_DB_FAIL;
	}

	return wStatus;
}

SMKY_STATUS SmkyAutoSubDatabase::init()
{
   return SMKY_STATUS_NO_MEMORY;
}

SmartKeyErrorCode SmkyAutoSubDatabase::save()
{
	return SmkyUserDatabase::smkyErrorToSmartKeyError(saveDb());
}

SMKY_STATUS SmkyAutoSubDatabase::saveDb()
{
	if (m_pDatabaseData == NULL)
		return SMKY_STATUS_INVALID_MEMORY;

	double start = SmartKeyService::getTime();

	std::string path = getDbPath();
	std::string tmpPath = path + ".tmp";

	std::string parentDir = SmartKeyService::dirName(tmpPath);
	if (!parentDir.empty()) {
		::g_mkdir_with_parents(parentDir.c_str(), S_IRWXU );
	}
	if (g_file_test(tmpPath.c_str(), G_FILE_TEST_EXISTS))
		g_unlink(tmpPath.c_str());

	SMKY_STATUS wStatus = SMKY_STATUS_WRITE_DB_FAIL;
	int fd = open(tmpPath.c_str(), O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR|S_IRGRP);
	if (-1 != fd) {
		ssize_t written = write(fd, m_pDatabaseData, k_DatabaseSize);
		fsync(fd);
		close(fd);
		if (size_t(written) == k_DatabaseSize) {
			if (0 == g_rename(tmpPath.c_str(), path.c_str())) {
				wStatus = SMKY_STATUS_NONE;
				double now = SmartKeyService::getTime();
				g_debug("Auto-sub database saved in %g msec.  %u", (now-start) * 1000.0, wStatus);
			}
			else {
				g_warning("Error writing auto-sub db");
				g_unlink(tmpPath.c_str());
			}
		}
	}

	return wStatus;
}

SmartKeyErrorCode SmkyAutoSubDatabase::forgetWord(const std::string& shortcut)
{
	SMKY_STATUS wStatus;
	wStatus = SMKY_STATUS_NO_MEMORY;

	return SmkyUserDatabase::smkyErrorToSmartKeyError(wStatus);
}

SmartKeyErrorCode SmkyAutoSubDatabase::setLocaleSettings(const LocaleSettings& localeSettings)
{
	// We don't do anything because when the locale changes the engine destroys and
	// recreates me with a new locale.
	return SKERR_SUCCESS;
}

SMKY_STATUS SmkyAutoSubDatabase::cacheLdbEntries()
{
	m_ldbEntries.clear();

	std::list<Entry> entries;
	SMKY_STATUS wStatus = loadEntries(StockEntries, entries);
	if (wStatus == SMKY_STATUS_NONE) {
		std::list<Entry>::const_iterator entry;
		for (entry = entries.begin(); entry != entries.end(); ++entry) {
			std::string lcshortut = StringUtils::utf8tolower(entry->shortcut);
			m_ldbEntries[lcshortut] = entry->substitution;
		}
	}

	g_debug("Loaded %u LDB entries", m_ldbEntries.size());
	return wStatus;
}

/**
 * Find the substitution (if there is one) for the given shortcut.
 *
 * @return The substitution mapped by the shortcut or an empty string if no mapping.
 */
std::string SmkyAutoSubDatabase::getLdbSubstitution(const std::string& shortcut) const
{
	std::string lcshortut = StringUtils::utf8tolower(shortcut);

	std::map<std::string,std::string>::const_iterator entry = m_ldbEntries.find(lcshortut);
	if (entry == m_ldbEntries.end())
		return "";
	else
		return entry->second;
}

/**
 * Find the substitution (if there is one) for the given shortcut.
 *
 * @return The substitution mapped by the shortcut or an empty string if no mapping.
 */
std::string SmkyAutoSubDatabase::getHardCodedSubstitution(const std::string& shortcut) const
{
	std::string lcshortut = StringUtils::utf8tolower(shortcut);

	std::map<std::string,std::string>::const_iterator entry = m_hardCodedEntries.find(lcshortut);
	if (entry == m_hardCodedEntries.end())
		return "";
	else
		return entry->second;
}

}

