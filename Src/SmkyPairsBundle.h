/* @@@LICENSE
*
*      Copyright (c) 2010-2013 Hewlett-Packard Development Company, L.P.
*      Copyright (c) 2013 LG Electronics
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

#ifndef SMKY_PAIRS_BUNDLE_H
#define SMKY_FILEPAIRS_BUNDLE_H

#include "Settings.h"
#include "SmkyFilePairs.h"

namespace SmartKey
{
/**
 * wrapper around SmkyFilePairs to support locale-independent and locale-dependent dictionaries.
 * idia is to combine locale independent dictionary with locale dependent using one interface.
 * m_independent_dict is for static words set and never changed.
 */
class SmkyPairsBundle
{
private:
    std::string m_used_locale;

    //locale-independent dictionary
    SmkyFilePairs m_independent_dict;

    //locale-dependent dictionary
    SmkyFilePairs m_dependent_dict;

public:

    virtual ~SmkyPairsBundle (void) {};

    //m_used_locale
    virtual std::string getUsedLocale (void);

    //load dictionary according to current locale settings
    virtual void load (std::string i_independent_dict, std::string i_dependent_dict);

    //save dictionary
    virtual bool save (std::string i_db_file);

    //size: number of pairs
    int size (void);

    //add pair
    virtual void add (std::string i_key, std::string i_value);

    //remove pair by key
    virtual bool remove (std::string i_key);

    //find
    virtual std::string find (const std::string& shortcut);

    //export
    virtual void exportToList (std::list<AutoSubDatabase::Entry>& o_entries);

protected:
    //release all allocated objects
    void _clean (void);

    //read pairs from text file and add them to m_dictionary
    void _importFileDB (std::string i_db_file);

};

/**
* <here is function description>
*
* @return string
*   <return value description>
*/
inline std::string SmkyPairsBundle::getUsedLocale (void)
{
    return(m_used_locale);
}

/**
* <here is function description>
*/
inline void SmkyPairsBundle::load (std::string i_independent_dict, std::string i_dependent_dict)
{
    m_used_locale = Settings::getInstance()->locale;
    m_dependent_dict.load(i_dependent_dict);

    if(!m_independent_dict.isInitialized())
        m_independent_dict.load(i_independent_dict);
}

/**
* <here is function description>
*
* @return bool
*   <return value description>
*/
inline bool SmkyPairsBundle::save (std::string i_dependent_dict)
{
    return( m_dependent_dict.save(i_dependent_dict) );
}

/**
* size: number of pairs
*
* @return int
*   <return value description>
*/
inline int SmkyPairsBundle::size (void)
{
    return (m_independent_dict.size() + m_dependent_dict.size());
}

/**
* <here is function description>
*/
inline void SmkyPairsBundle::add (std::string i_key, std::string i_value)
{
    m_dependent_dict.add(i_key, i_value);
}

/**
* <here is function description>
*/
inline bool SmkyPairsBundle::remove (std::string i_key)
{
    return(m_dependent_dict.remove(i_key));
}

/**
* <here is function description>
*
* @return string
*   <return value description>
*/
inline std::string SmkyPairsBundle::find (const std::string& shortcut)
{
    std::string retval = m_independent_dict.find(shortcut);
    if(retval.length() > 0)
        return( retval );

    return( m_dependent_dict.find(shortcut) );
}

/**
* <here is function description>
*/
inline void SmkyPairsBundle::exportToList (std::list<AutoSubDatabase::Entry>& o_entries)
{
    m_independent_dict.exportToList(o_entries);
    m_dependent_dict.exportToList(o_entries);
}

}

#endif

