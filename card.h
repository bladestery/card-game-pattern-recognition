//
//  card.h
//  Big Data
//
//  Created by Ben Ruktantichoke on 3/21/15.
//  Copyright (c) 2015 Ben Ruktantichoke. All rights reserved.
//

#ifndef Big_Data_card_h
#define Big_Data_card_h

#define NUM_RANKS 10

typedef enum SUIT {
    Heart,
    Spade,
    Diamond,
    Club
} SUIT;

struct card {
    SUIT suit;
    int rank;
    int id;     //unique id for each card in the standard deck used for hashing purposes
                //Spades -> Hearts -> Diamonds -> Clubs; AceS(1)->AceH(2)->AceD(3)->AceC(4)->...->KingC(52)
};

struct hand {
    struct card *card1;
    struct card *card2;
    struct card *card3;
    struct card *card4;
    struct card *card5;
    
    int rank;
    int count;
    
    struct hand *next; /* for database usage */
                       /* machine learning: hand with the same cards but different sequence */
};

struct entry {
    int id;
    int hash;
    struct entry *prev;
    struct entry *next;
    
    int count;               /* number hands with the same hash */
    struct hand *hand;
    
};

struct database {
    struct entry **bucket;
};

struct item {
    int order[5];
    
    //rotation?
    
    int rank;
    int count;
    struct item *next;
};

struct pattern {
    unsigned char bitmap[7];
    int order; /* 1 if order matters, 0 if order doesn't matter */
    struct item *item;
    struct pattern *next;
};

struct databank {
    struct pattern **cannister;
};

struct rank_info{
    int rank;
    int count;
    
    struct hand_info *first;
    struct hand_info *last;
    
    int mult;
    float dev_mult;
    int valid_mult;
    
    int suit_mult;
    float dev_suit;
    int valid_suit;
    
    float std_dev;
    float dev_dev;
    int valid_dev;
};

struct hand_info {
    int mult;
    int suit_mult;
    float std_dev;
    struct hand_info *next;
};

struct rankings {
    struct rank_info *board[NUM_RANKS];
    struct rank_info *highest_count;
    int finalized;
};

/* functions */

void assign_id(struct card *card);
struct entry *create_entry(struct hand *hand, int id);
struct database *create_database(void);
void insert_database(struct database *database, struct hand *hand, int *del);
void destroy_database(struct database *database);
int query(struct database *database, struct hand *hand, int *res);
struct pattern *hand_to_pattern(struct hand *hand);
struct databank *create_databank(void);
void insert_databank(struct databank *databank, struct pattern *pattern);
void destroy_databank(struct databank *databank);
int search(struct databank *databank, struct pattern *pattern, int *res);
struct rankings *create_rankings(void);
void destroy_rankings(struct rankings *rankings);
void update_rankings(struct rankings *rankings, struct hand *hand);
void finalize_rankings(struct rankings *rankings);
int find(struct rankings *rankings, struct hand *hand, int *res);

#endif
