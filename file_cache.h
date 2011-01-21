/*
 * Copyright 2011 Iouri Khramtsov.
 *
 * This software is available under Apache License, Version 
 * 2.0 (the "License"); you may not use this file except in 
 * compliance with the License. You may obtain a copy of the
 * License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

// A set of classes responsible for loading files into
// cache and storing them there.
#ifndef ISAWORD_FILE_CACHE_H
#define ISAWORD_FILE_CACHE_H

#include <string>
#include <time.h>

#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/functional/hash.hpp>
#include <google/dense_hash_map>

namespace isaword {

class CachedFile;
typedef boost::shared_ptr<CachedFile> CachedFilePtr;

/**
 * A functor used to compare the strings in the hash set.
 */
struct eqstr {
    bool operator()(const std::string& first, const std::string& second) const {
        return (first == second);
    }
};

/*---------------------------------------------------------
                    FileCache class.
----------------------------------------------------------*/
class FileCache {
public:
    /**
     * The default cache expiration period, in sec.
     */
    static const time_t kDefaultExpirationPeriod = 60;
    
    FileCache()
    : expiration_period_(kDefaultExpirationPeriod) {
        cached_files_.set_empty_key("");
    }
    
    FileCache(time_t expiration_period)
    : expiration_period_(expiration_period) {
        cached_files_.set_empty_key("");
    }
    
    /**
     * Get the contents of the file.
     *
     * The data will be returned in the first argument, and will be always 
     * zero terminated.  The second optional argument can be used to return
     * the number of bytes of data returned.
     *
     * The function will return true on success, false on failure.
     */
    bool get(const std::string& file_path, 
             boost::shared_array<char>& data, 
             size_t* data_size = NULL);
    
    /*=============== Getters/Setters ====================*/
    /// Get the cache expiration period.
    time_t expiration_period() const                {return expiration_period_;}
    
    /// Set the cache expiration period.
    void set_expiration_period(time_t period)       {expiration_period_ = period;}

private:
    typedef google::dense_hash_map<std::string, 
                                   CachedFilePtr, 
                                   boost::hash<std::string>, 
                                   eqstr>
            CachedFilesMap;
    
    /// Cached files.
    CachedFilesMap cached_files_;
    
    /// Cache expiration period.
    time_t expiration_period_;
};


/*---------------------------------------------------------
                    CachedFile class.
----------------------------------------------------------*/
class CachedFile {
public:
    /**
     * The default cache expiration period, in sec.
     */
    static const time_t kDefaultExpirationPeriod = 60;
    
    CachedFile(const std::string& file_path)
    : data_(NULL), 
      data_size_(0), 
      data_capacity_(0), 
      expiration_time_(0),
      expiration_period_(kDefaultExpirationPeriod),
      last_modified_(0),
      file_path_(file_path) {
    }
    
    CachedFile(const std::string& file_path, time_t expiration_period)
    : data_(NULL), 
      data_size_(0), 
      data_capacity_(0), 
      expiration_time_(0),
      expiration_period_(expiration_period),
      last_modified_(0),
      file_path_(file_path) {
    }
    
    /**
     * Get the data.
     * Returns true if the data exists, false if the file is not accessible.
     * The data will be returned the array data; the size of the data will be
     * returned in the second argument (size).  Data will always be null-
     * terminated to allow string operations; in some cases the '\0' character 
     * may be past the limit returned in the size argument.
     * 
     * Returns false if the file does not exist or has been deleted.
     */
    bool get(boost::shared_array<char>& data, size_t& size);
    
    /*=============== Getters/Setters ====================*/
    /// Get the cache expiration time.
    time_t expiration_time() const                  {return expiration_time_;}
    
    /// Get the cache expiration period.
    time_t expiration_period() const                {return expiration_period_;}
    
    /// Set the cache expiration period.
    void set_expiration_period(time_t period)       {expiration_period_ = period;}
    
    /// Get the name of the cached file.
    std::string file_path() const                   {return file_path_;}
    
private:
    /**
     * A helper function for blanking out the data.
     */
    void empty_data();
    
    /**
     * The data stored here.
     */
    boost::shared_array<char> data_;
    
    /**
     * Bytes of data stored.
     */
    size_t data_size_;
    
    /**
     * The total capacity of the data_ array.
     */
    size_t data_capacity_;
    
    /**
     * Time when the cached file expires.
     */
    time_t expiration_time_;
    
    /**
     * Cache expiration period, in seconds.
     */
    time_t expiration_period_;
    
    /**
     * The last time the file on disk was modified.
     */
    time_t last_modified_;
    
    /**
     * File name.
     */
    std::string file_path_;
};

} /* namespace isaword */
#endif
