/*
        Flexisip, a flexible SIP proxy server with media capabilities.
    Copyright (C) 2012  Belledonne Communications SARL.
    Author: Guillaume Beraudo

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef registrardb_redis_hh
#define registrardb_redis_hh

#include "registrardb.hh"
#include "recordserializer.hh"
#include <sofia-sip/sip.h>
#include <sofia-sip/nta.h>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include "agent.hh"

struct RedisParameters {
        RedisParameters() : port(0), timeout(0) {}
        std::string domain;
        std::string auth;
        int port;
        int timeout;
};

class RegistrarDbRedisAsync : public RegistrarDb {
public:
        struct RegistrarUserData;
        RegistrarDbRedisAsync(const string &preferredRoute, su_root_t* root, RecordSerializer* serializer, RedisParameters params);

protected:
        bool connect();
        virtual void doBind(const BindParameters &params, const std::shared_ptr<RegistrarDbListener> &listener);
        virtual void doClear(const sip_t *sip, const std::shared_ptr<RegistrarDbListener> &listener);
        virtual void doFetch(const url_t *url, const std::shared_ptr<RegistrarDbListener> &listener);


private:
        RegistrarDbRedisAsync(Agent *agent, RedisParameters params);
        ~RegistrarDbRedisAsync();
        static void connectCallback(const redisAsyncContext *c, int status);
        static void disconnectCallback(const redisAsyncContext *c, int status);
        bool isConnected();
        friend class RegistrarDb;
        redisAsyncContext *mContext;
        RecordSerializer *mSerializer;
        std::string sDomain;
        std::string sAuthPassword;
        int sPort;
        int sTimeout;
        su_root_t *mRoot;
        unsigned long mToken;
        unsigned long getToken();
        static void sHandleSet(redisAsyncContext* ac, void *r, void *privdata);
        static void sHandleAorGetReply(struct redisAsyncContext*, void *r, void *privdata);
        static void sHandleBind(redisAsyncContext* ac, redisReply *reply, RegistrarUserData *data);
        static void sHandleFetch(redisAsyncContext* ac, redisReply *reply, RegistrarUserData *data);
        static void sHandleClear(redisAsyncContext* ac, redisReply *reply, RegistrarUserData *data);
        void handleClear(redisReply *reply, RegistrarUserData *data);
        void handleFetch(redisReply *reply, RegistrarUserData *data);
        void handleBind(redisReply *reply, RegistrarUserData *data);
        void onBindReplyAorSet(redisReply *reply, RegistrarUserData *data);

};


#endif
