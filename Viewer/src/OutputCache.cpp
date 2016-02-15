//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "OutputCache.hpp"

#include "UserMessage.hpp"

OutputCache* OutputCache::instance_=NULL;

#define _UI_OUTPUTCACHE_DEBUG

OutputCacheItem::OutputCacheItem(const std::string& id, VFile_ptr file,QObject *parent) :
      QTimer(parent), id_(id), file_(file), timeout_(15*1000), cnt_(0)
{
    attach();
}

bool OutputCacheItem::sameAs(VInfo_ptr info,const std::string& sourcePath)
{
    if(info && info->isNode() && info->server())
    {
        std::string id=info->path() + ":" + sourcePath;
        return (id == id_);
    }
    return false;
}

/*void OutputCacheItem::markForRemove(bool forget)
{
    forget_=forget;
    start(timeout_);
}*/

void OutputCacheItem::attach()
{
    cnt_++;
    stop();
}

void OutputCacheItem::detach()
{
    cnt_--;
    if(cnt_<=0)
    {
        cnt_=0;
        start(timeout_);
    }
}

/*void OutputCacheItem::keepIt()
{
    forget_=false;
    stop();
}*/

OutputCache::~OutputCache()
{
   clear();
}

OutputCache* OutputCache::instance()
{
    if(!instance_)
        instance_= new OutputCache();
    
    return instance_;
}

void OutputCache::clear()
{
    Q_FOREACH(OutputCacheItem* item,items_.values())
    {
        delete item;
    }
}

OutputCacheItem* OutputCache::add(VInfo_ptr info,const std::string& sourcePath,VFile_ptr file)
{
    if(!file)
        return NULL;

#ifdef _UI_OUTPUTCACHE_DEBUG
    UserMessage::debug("OutputCache::add --> file");
    file->print();
    print();
#endif

    if(info && file  && info->isNode() && info->server())
    {
        std::string id=info->path() + ":" + sourcePath;        
        QMap<std::string, OutputCacheItem*>::iterator it = items_.find(id);
        if(it != items_.end())
        {
            it.value()->attach();
            it.value()->file_=file;
            return it.value();
        }
        else
        {
            OutputCacheItem* item=new OutputCacheItem(id,file);
            connect(item,SIGNAL(timeout()),
                    this,SLOT(removeItem()));
            
            items_[id]=item;
#ifdef _UI_OUTPUTCACHE_DEBUG
            UserMessage::debug("  add item:" + id);
            print();
            UserMessage::debug("<-- OutputCache::add");
            return item;
#endif
        }
    }
#ifdef _UI_OUTPUTCACHE_DEBUG
    UserMessage::debug("<-- OutputCache::add");
#endif

    return NULL;
}    

void OutputCache::detach(OutputCacheItem* item)
{
    if(item)
    {
#ifdef _UI_OUTPUTCACHE_DEBUG
        UserMessage::debug("OutputCache::detach -->");
        print();
        UserMessage::debug("  detach item: " + item->id_);
        item->detach();
        print();
        UserMessage::debug("<-- OutputCache::detach");
#endif
    }
}

/*
void OutputCache::markForRemove(VFile_ptr file,bool forget)
{
    if(!file)
        return;
#ifdef _UI_OUTPUTCACHE_DEBUG
    UserMessage::debug("OutputCache::markForRemove --> ref_count: " +
                       QString::number(file.use_count()).toStdString());
    file->print();
    print();
#endif

    QMap<std::string, OutputCacheItem*>::iterator it = items_.begin();
    while (it != items_.end() ) 
    {
       if(it.value()->file_ == file)
       {    
           it.value()->markForRemove(forget);
#ifdef _UI_OUTPUTCACHE_DEBUG
           UserMessage::debug("  item to remove: " + it.key());
           print();
#endif
           return;
       }    
       
       ++it;
    }    
}
*/

void OutputCache::removeItem()
{
    if(OutputCacheItem* item=static_cast<OutputCacheItem*>(sender()))
    {
#ifdef _UI_OUTPUTCACHE_DEBUG
        UserMessage::debug("OutputCache::removeItem --> file");
        item->file_->print();
        print();
#endif
        QMap<std::string, OutputCacheItem*>::iterator it = items_.begin();
        while (it != items_.end() ) 
        {
            if(it.value() == item)
            {            
#ifdef _UI_OUTPUTCACHE_DEBUG
                UserMessage::debug("  remove item --> ref_count:" +
                                     QString::number(it.value()->file_.use_count()).toStdString() +
                                     " item:" + it.key());
#endif
                //assert(item->file_.use_count() == 1);
                //The ref count is not necessarily 1 here
                items_.erase(it);
                item->deleteLater();
#ifdef _UI_OUTPUTCACHE_DEBUG
                print();
                UserMessage::debug("<-- OutputCache::removeItem");
#endif
                return;
            }

            ++it;
        }
    }        
}
/*
bool OutputCache::isCurrent(VInfo_ptr info,const std::string& sourcePath)
{
    if(info && info->isNode() && info->server())
    {
        std::string id=info->path() + ":" + sourcePath;
        QMap<std::string, OutputCacheItem*>::iterator it = items_.find(id);
        return (it != items_.end() && it.value() == current_);
    }

    return false;
}

VFile_ptr OutputCache::makeCurent(VInfo_ptr info,const std::string& sourcePath)
{
    if(info && info->isNode() && info->server())
    {
        std::string id=info->path() + ":" + sourcePath;
        QMap<std::string, OutputCacheItem*>::iterator it = items_.find(id);
        if(it != items_.end())
        {
            if(it.value() != current_)
            {
                current_->markForRemove();
                current_=it.value();
            }
            current_->keepIt();
            return it.value()->file_;
        }
    }

    return VFile_ptr();
}
*/
/*
VFile_ptr OutputCache::find(VInfo_ptr info,const std::string& sourcePath)
{
    if(info && info->isNode() && info->server())
    {
        std::string id=info->path() + ":" + sourcePath;        
        QMap<std::string, OutputCacheItem*>::iterator it = items_.find(id);       
        if(it != items_.end())
        {
            if(it.value()->forget_)
                return  VFile_ptr();
            it.value()->keepIt();
            return it.value()->file_;
        }
    }

    return VFile_ptr();
}    
*/
OutputCacheItem* OutputCache::use(VInfo_ptr info,const std::string& sourcePath)
{
    if(info && info->isNode() && info->server())
    {
        std::string id=info->path() + ":" + sourcePath;
        QMap<std::string, OutputCacheItem*>::iterator it = items_.find(id);
        if(it != items_.end())
        {
            it.value()->attach();
            return it.value();

        }
    }

    return NULL;
}

void OutputCache::print()
{
    UserMessage::debug("  OutputCache contents -->");
    QMap<std::string, OutputCacheItem*>::iterator it = items_.begin();
    while (it != items_.end() )
    {
        UserMessage::debug("  item:" + it.key() + " tmp:" +
                             it.value()->file_->path() + " countdown:" +
                             ((it.value()->isActive())?"on":"off"));
        ++it;
    }
    UserMessage::debug("  <-- OutputCache contents");
}
