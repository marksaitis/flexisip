#include "recordserializer.hh"
#include "registrardb-redis.hh"
#include "common.hh"

#include <algorithm>

#include "configmanager.hh"

#include <hiredis/hiredis.h>

#include <sofia-sip/sip_protos.h>
#include <sofia-sip/su_wait.h>

#include <memory>

using namespace std;

bool sUseSyslog;

struct DumpListener : public RegistrarDbListener {

private:
	su_root_t* root;

private:
	void su_break(){
		if( root ){
			su_root_break(root);
		}
	}
	
public:
	bool listenerError;

public:
	DumpListener(su_root_t* _root) : RegistrarDbListener(), root(_root), listenerError(false) {}
	
	virtual void onRecordFound(Record *record) {
		if (record) cout << *record           << endl;
		else        cout << "No record found" << endl;
		su_break();
	}
	virtual void onError() {
		SLOGE << "Connection error, aborting" <<  endl;
		listenerError = true;
		su_break();
	}
	virtual void onInvalid() {
		SLOGW << "Invalid" << endl;
		listenerError = true;
		su_break();
	}
};

struct CTArgs {
	bool debug;
	RedisParameters redis;
	string serializer;
	string url;

	static void usage(const char *app) {
		CTArgs args;
		cout << app << " -t host[" << args.redis.domain << "] "
			<< "-p port[" << args.redis.port << "] "
			<< "--debug "
			<< "-a auth "
			<< "-s serializer[" << args.serializer << "] "
			<< "sip_uri " << endl;
	}
	
	CTArgs() {
		debug = false;
		redis.port=6379;
		redis.timeout = 2000;
		redis.domain = "sip";
		serializer = "protobuf";
	}

	void parse(int argc, char **argv) {
		#define EQ0(i, name) (strcmp(name, argv[ i ]) == 0)
		#define EQ1(i, name) (strcmp(name, argv[ i ]) == 0 && argc > i)
		for (int i = 1; i < argc; ++i) {
			if (EQ0(i, "--debug")) {
				debug = true;
			} else if (EQ0(i, "--help") || EQ0(i, "-h")) {
				usage(*argv);
				exit(0);
			} else if (EQ1(i, "-p")) {
				redis.port = atoi(argv[++i]);
			} else if (EQ1(i, "-t")) {
				redis.domain = argv[++i];
			} else if (EQ1(i, "-a")) {
				redis.auth = argv[++i];
			} else if (EQ1(i, "-s")) {
				serializer = argv[++i];
				if (serializer != "protobuf" && serializer != "c" && serializer != "json") {
					cerr << "invalid serializer : " << serializer << endl;
					exit(-1);
				}
			} else {
				url = argv[i++];
				if (argc > i) {
					cerr << "? arg" << i << " " << argv[i] << endl;
					usage(*argv);
					exit(-1);
				}
			}
		}
		if (url.empty()) {
			cerr << "specify aor" << endl;
			usage(*argv);
			exit(-1);
		}
	}
};

static void timerfunc(su_root_magic_t *magic, su_timer_t *t, Agent *arg){
	arg->idle();
}

int main(int argc, char **argv) {
	su_home_t home;
	su_root_t* root;
	sUseSyslog = false;
	shared_ptr<Agent> agent;
	CTArgs args; args.parse(argc, argv);

	flexisip::log::preinit(sUseSyslog, args.debug);
	flexisip::log::initLogs(sUseSyslog, args.debug);
	flexisip::log::updateFilter("%Severity% >= debug");

	Record::sLineFieldNames = {"line"};
	Record::sMaxContacts = 10;

	su_home_init(&home);
	root = su_root_create(NULL);
	agent      = make_shared<Agent>(root);

	auto serializer  = unique_ptr<RecordSerializer>(RecordSerializer::create(args.serializer));
	auto registrardb = new RegistrarDbRedisAsync("localhost", root, serializer.get(), args.redis);
	auto url = url_format(&home,args.url.c_str());
	auto listener = make_shared<DumpListener>(root);
	
	registrardb->fetch(url, listener);
	
	su_timer_t* timer = su_timer_create(su_root_task(root),5000);
	if( !listener->listenerError ){
		su_timer_set_for_ever(timer,(su_timer_f)timerfunc,agent.get());
		su_root_run(root);
	}
	
	agent.reset();
	
	su_timer_destroy(timer);
	su_root_destroy(root);
	su_home_destroy(&home);
}
