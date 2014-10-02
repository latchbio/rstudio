/*
 * SourceIndex.cpp
 *
 * Copyright (C) 2009-12 by RStudio, Inc.
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#include "SourceIndex.hpp"

#include <boost/foreach.hpp>

#include <core/FilePath.hpp>
#include <core/PerformanceTimer.hpp>

#include <core/system/ProcessArgs.hpp>

#include "SharedLibrary.hpp"
#include "UnsavedFiles.hpp"

using namespace core ;

namespace session {
namespace modules { 
namespace clang {
namespace libclang {

bool SourceIndex::isTranslationUnit(const std::string& filename)
{
   std::string ex = FilePath(filename).extensionLowerCase();
   return (ex == ".c" || ex == ".cc" || ex == ".cpp" ||
           ex == ".m" || ex == ".mm");
}

SourceIndex::SourceIndex()
   : index_(NULL), verbose_(false)
{
}

SourceIndex::~SourceIndex()
{
   try
   {
      // remove all
      removeAllTranslationUnits();

      // dispose the index
      if (index_ != NULL)
         clang().disposeIndex(index_);
   }
   catch(...)
   {
   }
}

void SourceIndex::initialize(CompilationDatabase compilationDB, int verbose)
{
   verbose_ = verbose;
   index_ = clang().createIndex(0, (verbose_ > 0) ? 1 : 0);
   compilationDB_ = compilationDB;
}

unsigned SourceIndex::getGlobalOptions() const
{
   return clang().CXIndex_getGlobalOptions(index_);
}

void SourceIndex::setGlobalOptions(unsigned options)
{
   clang().CXIndex_setGlobalOptions(index_, options);
}

void SourceIndex::removeTranslationUnit(const std::string& filename)
{
   TranslationUnits::iterator it = translationUnits_.find(filename);
   if (it != translationUnits_.end())
   {
      if (verbose_ > 0)
         std::cerr << "CLANG REMOVE INDEX: " << it->first << std::endl;
      clang().disposeTranslationUnit(it->second.tu);
      translationUnits_.erase(it->first);
   }
}

void SourceIndex::removeAllTranslationUnits()
{
   for(TranslationUnits::const_iterator it = translationUnits_.begin();
       it != translationUnits_.end(); ++it)
   {
      if (verbose_ > 0)
         std::cerr << "CLANG REMOVE INDEX: " << it->first << std::endl;

      clang().disposeTranslationUnit(it->second.tu);
   }

   translationUnits_.clear();
}


void SourceIndex::primeTranslationUnit(const std::string& filename)
{
   // if we have no record of this translation unit then do a first pass
   if (translationUnits_.find(filename) == translationUnits_.end())
      getTranslationUnit(filename);
}

void SourceIndex::reprimeTranslationUnit(const std::string& filename)
{
   // if we have already indexed this translation unit then re-index it
   if (translationUnits_.find(filename) != translationUnits_.end())
      getTranslationUnit(filename);
}


TranslationUnit SourceIndex::getTranslationUnit(const std::string& filename)
{
   // header files get their own codepath
   if (!SourceIndex::isTranslationUnit(filename))
      return getHeaderTranslationUnit(filename);

   FilePath filePath(filename);

   boost::scoped_ptr<core::PerformanceTimer> pTimer;
   if (verbose_ > 0)
   {
      std::cerr << "CLANG INDEXING: " << filePath.absolutePath() << std::endl;
      pTimer.reset(new core::PerformanceTimer(filePath.filename()));
   }

   // get the arguments and last write time for this file
   std::vector<std::string> args;
   if (!compilationDB_.empty())
      args = compilationDB_.compileArgsForTranslationUnit(filename);
   std::time_t lastWriteTime = filePath.lastWriteTime();

   // look it up
   TranslationUnits::iterator it = translationUnits_.find(filename);

   // check for various incremental processing scenarios
   if (it != translationUnits_.end())
   {
      // alias record
      StoredTranslationUnit& stored = it->second;

      // already up to date?
      if (args == stored.compileArgs && lastWriteTime == stored.lastWriteTime)
      {
         return TranslationUnit(filename, stored.tu);
      }

      // just needs reparse?
      else if (args == stored.compileArgs)
      {
         int ret = clang().reparseTranslationUnit(
                                stored.tu,
                                unsavedFiles().numUnsavedFiles(),
                                unsavedFiles().unsavedFilesArray(),
                                clang().defaultReparseOptions(stored.tu));

         if (ret == 0)
         {
            // update last write time
            stored.lastWriteTime = lastWriteTime;

            // return it
            return TranslationUnit(filename, stored.tu);
         }
         else
         {
            LOG_ERROR_MESSAGE("Error re-parsing translation unit " + filename);
         }
      }
   }

   // if we got this far then there either was no existing translation
   // unit or we require a full rebuild. in all cases remove any existing
   // translation unit we have
   removeTranslationUnit(filename);

   // add verbose output if requested
   if (verbose_ >= 2)
     args.push_back("-v");

   // get the args in the fashion libclang expects (char**)
   core::system::ProcessArgs argsArray(args);

   // create a new translation unit from the file
   CXTranslationUnit tu = clang().parseTranslationUnit(
                         index_,
                         filename.c_str(),
                         argsArray.args(),
                         argsArray.argCount(),
                         unsavedFiles().unsavedFilesArray(),
                         unsavedFiles().numUnsavedFiles(),
                         clang().defaultEditingTranslationUnitOptions());


   // save and return it if we succeeded
   if (tu != NULL)
   {
      translationUnits_[filename] = StoredTranslationUnit(args,
                                                          lastWriteTime,
                                                          tu);

      return TranslationUnit(filename, tu);
   }
   else
   {
      LOG_ERROR_MESSAGE("Error parsing translation unit " + filename);
      return TranslationUnit();
   }
}

TranslationUnit SourceIndex::getHeaderTranslationUnit(
                                             const std::string& filename)
{
   // scan through our existing translation units for this file
   for(TranslationUnits::const_iterator it = translationUnits_.begin();
       it != translationUnits_.end(); ++it)
   {
      TranslationUnit tu(it->first, it->second.tu);
      if (tu.includesFile(filename))
         return tu;
   }

   // drats we don't have it! we can still try to index other src files
   // in search of one that includes this header
   std::vector<std::string> srcFiles;
   if (!compilationDB_.empty())
      srcFiles = compilationDB_.translationUnits();
   BOOST_FOREACH(const std::string& filename, srcFiles)
   {
      TranslationUnit tu = getTranslationUnit(filename);
      if (!tu.empty())
      {
         // found it! (keep it in case we need it again)
         if (tu.includesFile(filename))
         {
            return tu;
         }
         // didn't find it (dispose it to free memory)
         else
         {
            removeTranslationUnit(filename);
         }
      }
   }

   return TranslationUnit();
}



// singleton
SourceIndex& sourceIndex()
{
   static SourceIndex instance;
   return instance;
}

} // namespace libclang
} // namespace clang
} // namespace modules
} // namesapce session

