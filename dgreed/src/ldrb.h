#ifndef LDRB_H
#define LDRB_H

// ldrb - leaderboards database

// Init callback type
typedef void (*LdrbInitCallback)(success);

// Initializes ldrb and tries to connect to server.
// gid - game id, uid - user id. cb is called either
// when ldrb is successfully initialized or there
// was an error (for example, server can't be reached).
void ldrb_init(const char* server, const char* gid, const char* uid,
		LdrbInitCallback cb);

// Close ldrb
void ldrb_close();

// You don't always know uid on startup - get/set gid anytime using these
const char* ldrb_uid(void);
void ldrb_set_uid(const char* uid);

typedef struct {
	int flag;			// Arbitrary flag (a level number, for example)
    int weight;			// Weight, by which entries are ordered
    size_t data_size;	
    void* data;
} LdrbEntry;

typedef struct {
	int flag;
    int start_entry;
    int entry_count;
    time_t start_time;
    time_t end_time;
} LdrbQuery;

// Puts entry into database.
void ldrb_put(const LdrbEntry* entry);

// Request callback type
typedef void (*LdrbCallback)(const LdrbEntry* entries, int n_entries);

// Makes a query for ordered list of entries
void ldrb_query(const LdrbQuery* query, LdrbCallback* cb);

#endif

