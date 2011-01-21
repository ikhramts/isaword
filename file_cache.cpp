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

#include <errno.h>
#include <fstream>
#include <time.h>
#include <string>
#include <sys/stat.h>
#include <boost/shared_array.hpp>
#include <google/dense_hash_map>

#include "file_cache.h"

using boost::shared_array;
using boost::shared_ptr;

namespace isaword {

/*---------------------------------------------------------
                    FileCache class.
----------------------------------------------------------*/
/**
 * Get the contents of the file.
 *
 * The data will be returned in the first argument, and will be always 
 * zero terminated.  The second optional argument can be used to return
 * the number of bytes of data returned.
 *
 * @return will return true on success, false on failure.
 */
bool FileCache::get(const std::string& file_path,
                    boost::shared_array<char>& data, 
                    size_t* data_size) {
    //Check if the file is already being cached.
    CachedFilesMap::const_iterator it = cached_files_.find(file_path);
    CachedFilePtr cached_file;
    
    if (it == cached_files_.end()) {
        //Start caching the file.
        cached_file = CachedFilePtr(new CachedFile(file_path, expiration_period_));
        cached_files_[file_path] = cached_file;
    
    } else {
        cached_file = it->second;
    }
    
    //Get the file contents.
    size_t temp_data_size = 0;
    const bool found_file = cached_file->get(data, temp_data_size);
    
    if (data_size != NULL) {
        *data_size = temp_data_size;
    }
    
    return found_file;
}
    
/*---------------------------------------------------------
                    CachedFile class.
----------------------------------------------------------*/
const time_t CachedFile::kDefaultExpirationPeriod;

/**
 * Get the data.
 * Returns true if the data exists, false if the file is not accessible.
 * The data will be returned the array data; the size of the data will be
 * returned in the second argument (size).
 * 
 * Returns false if the file does not exist or has been deleted.
 */
bool CachedFile::get(boost::shared_array<char>& data, size_t& size) {
    //Check whethr the cache has expired or nonexistent.
    const time_t now = time(NULL);
    shared_array<char> empty_array;
    
    if (now >= expiration_time_ || data_ == empty_array) {
        expiration_time_ = now + expiration_period_;
        
        //Reload the cache.
        //Read the file size.
        struct stat stat_buffer;
        int status = stat(file_path_.c_str(), &stat_buffer);
       
        if (status != 0 || S_ISDIR(stat_buffer.st_mode)) {
            this->empty_data();
            goto return_data;
        }
        
        //Check whether the file has changed since the last time we
        //read it.
        if (stat_buffer.st_mtime == last_modified_) {
            goto return_data;
        
        } else {
            last_modified_ = stat_buffer.st_mtime;
        }

        //Attempt to open the file.
        std::ifstream file(file_path_.c_str());
        if (!file.good()) {
            file.close();
            this->empty_data();
            goto return_data;
        }

        
        size_t file_size = stat_buffer.st_size;
        //shared_array<char> data(new char[file_size]);
        //data_ = data;
        data_ = shared_array<char>(new char[file_size + 1]);
        data_capacity_ = file_size;
        
        //Read the file.
        data_size_ = static_cast<size_t>(file.readsome(data_.get(), data_capacity_));
        file.close();
        
        //Terminate the data string with a zero.
        data_[data_size_] = '\0';
    }
    
return_data:
    data = data_;
    size = data_size_;
    return (data_ != empty_array);
}

/**
 * A helper function for blanking out the data.
 */
void CachedFile::empty_data() {
    shared_array<char> empty_array;
    data_ = empty_array;
    data_capacity_ = 0;
    data_size_ = 0;
}

} /* namespace isaword */
