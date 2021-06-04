#include <stdio.h>   /* standard I/O routines                 */
#include <pthread.h> /* pthread functions and data structures */
#include <stdlib.h>
#define NUM_PLAYERS 4 /* size of each array.    */
#define MAX_PLAYER_HEIGHT 190
#define MIN_PLAYER_HEIGHT 160

#define MAX_BALL_HEIGHT 250
#define MIN_BALL_HEIGHT 10

#define MAX_JUMP_HEIGHT 50
#define MIN_JUMP_HEIGHT 10

// define colors
#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"
// end of colors

/* global mutex for our program. assignment initializes it */
pthread_mutex_t a_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t throw_ball = PTHREAD_COND_INITIALIZER;
pthread_cond_t catch_ball = PTHREAD_COND_INITIALIZER;
pthread_cond_t referee = PTHREAD_COND_INITIALIZER;

int x = 0;

struct player /* player struct have thew players info */
{
    int id;
    int height;
    int num_seekers; /* number of times the player was seeker */
};
struct game
{
    int seeker_id;
    int thrower_id;
    int ball_height;
    int seeker_height;
    int next_player_id;
};

struct game game_g = {0, 0, 0, 0, 0}; /* initialize the game info */

struct player players[] = {{1, 0, 0}, {2, 0, 0}, {3, 0, 0}, {4, 0, 0}}; /* initialize the players  */

/* function to be executed by the variable setting threads thread */
void *player_code(void *data)
{

    while (1)
    {

        usleep(2000);

        int rc; /* contain mutex lock/unlock results */
        int receiver;
        int height = 0, height1 = 0, height2 = 0; /* heights variables */
        int thid = ((struct player *)data)->id;   /* thread identifying number         */
        int thrower = game_g.thrower_id;          /* throwser id */
        int seeker = game_g.seeker_id;            /* seeker id */
        int is_jump;                              /* flag to ckeck if the player jump or not  */
        if (thrower == thid)                      /* statment to check if the player is thrower or seeker */
        {
            /* throwser code starts here */

            rc = pthread_mutex_lock(&a_mutex); /* lock the mutex, to assure exclusive access to game_g . */
            if (game_g.ball_height != 0)       /* check if the seeker try to catck the ball or not */
            {
                rc = pthread_mutex_unlock(&a_mutex);
                continue;
            }

            do
            {
                receiver = rand() % NUM_PLAYERS + 1;
            } while (receiver == thrower || receiver == seeker);

            game_g.next_player_id = receiver;

            is_jump = rand() % 2;

            if (is_jump)
            {
                height1 = (rand() % (MAX_BALL_HEIGHT - MIN_BALL_HEIGHT)) + MIN_BALL_HEIGHT;
                height2 = (rand() % (MAX_JUMP_HEIGHT - MIN_JUMP_HEIGHT)) + MIN_JUMP_HEIGHT;
                height = height1 + height2 + players[thid - 1].height;
            }
            else
            {
                int height1 = (rand() % (MAX_BALL_HEIGHT - MIN_BALL_HEIGHT)) + MIN_BALL_HEIGHT;
                height = height1 + players[thid - 1].height;
            }

            game_g.ball_height = height;

            printf("\t   %sPlayer %d throw the ball forword player %d with height %d%s\n", KCYN, thid, receiver, height, KNRM);
            fflush(stdout);

            rc = pthread_mutex_unlock(&a_mutex);
        }
        else if (game_g.seeker_id == thid)
        {
            /* lock the mutex, to assure exclusive access to game_g . */
            rc = pthread_mutex_lock(&a_mutex);
            if (game_g.ball_height == 0 || game_g.seeker_height != 0) /* check if the thrower throws the ball or not */
            {
                rc = pthread_mutex_unlock(&a_mutex);
                continue;
            }

            is_jump = rand() % 2;
            if (is_jump)
            {
                height1 = (rand() % (MAX_JUMP_HEIGHT - MIN_JUMP_HEIGHT)) + MIN_JUMP_HEIGHT;
                height = height1 + players[thid - 1].height;
            }
            else
            {
                height = players[thid - 1].height;
            }
            printf("\t   %sPlayer %d 'seeker' try to catch the ball his height is %d%s\n", KMAG, thid, height, KNRM);
            fflush(stdout);
            game_g.seeker_height = height;
            rc = pthread_mutex_unlock(&a_mutex);
        }
        pthread_mutex_lock(&a_mutex);

        if (x == 0 && game_g.ball_height != 0 && game_g.seeker_height != 0)
        {
            x = 1;
            pthread_cond_signal(&referee);
        }
        else if (x == 1)
        {
            x = 0;
        }
        pthread_mutex_unlock(&a_mutex);
    }
}
void *referee_code(void *data)
{
    int thid = (int *)data;
    while (1)
    {
        pthread_mutex_lock(&a_mutex);
        pthread_cond_wait(&referee, &a_mutex);

        if (game_g.ball_height > game_g.seeker_height)
        {
            game_g.thrower_id = game_g.next_player_id;

            printf("\n\t\t    %sThe seeker failed to catch the ball%s\n\n", KRED, KNRM);
            fflush(stdout);
        }
        else
        {
            players[game_g.thrower_id - 1].num_seekers++;
            int tmp = game_g.thrower_id;
            game_g.thrower_id = game_g.seeker_id;
            game_g.seeker_id = tmp;
            printf("\n\t\t%sThe seeker catch the ball and become the thrower\n%s\n", KGRN, KNRM);
            fflush(stdout);
            printf("%s\t\t   [player1:%d,player2:%d,player3:%d,player4:%d]%s\n\n", KYEL, players[0].num_seekers, players[1].num_seekers, players[2].num_seekers, players[3].num_seekers, KNRM);
            fflush(stdout);
            if (players[game_g.seeker_id - 1].num_seekers == 5)
            {
                printf("\n\n\t--------------------%sPlayer %d is the loser%s--------------------\n", KBLU, game_g.seeker_id, KNRM);
                fflush(stdout);
                break;
            }
        }
        game_g.ball_height = 0;
        game_g.seeker_height = 0;
        pthread_mutex_unlock(&a_mutex);
    }
    exit(1);
}

int main(int argc, char *argv[])
{

    int i;       /* loop counter                          */
    int thr_id1; /* thread ID for the first new thread    */
    int thr_id2; /* thread ID for the second new thread   */
    int thr_id3; /* thread ID for the theird new thread   */
    int thr_id4; /* thread ID for the fourth new thread   */
    int thr_id5; /* thread ID for the fifth new thread   */

    int seeker;
    int thrower;

    pthread_t p_thread[5]; /*  threads structure              */

    int num1 = 1; /* thread 1 player number              */
    int num2 = 2; /* thread 2 player number              */
    int num3 = 3; /* thread 3 player number              */
    int num4 = 4; /* thread 4 player number              */
    int num5 = 5; /* thread 5 player number              */

    srand((unsigned)getpid());

    thrower = rand() % NUM_PLAYERS + 1; /* pick a random thrower player */

    do
    {
        seeker = rand() % NUM_PLAYERS + 1;
    } while (thrower == seeker); /* pick random seeker player  */

    game_g.seeker_id = seeker;
    game_g.thrower_id = thrower;

    for (int i = 0; i < NUM_PLAYERS; i++) /* define a random talls for the players */
    {
        players[i].height = (rand() % (MAX_PLAYER_HEIGHT - MIN_PLAYER_HEIGHT)) + MIN_PLAYER_HEIGHT;
    }

    players[seeker - 1].num_seekers++; /* Add 1 point to the initialy picked seeker player */

    printf("\n\t---------Done By Nemer Tamimi & Jehad Jitan & Abdelaziz Bayanti---------\n");
    printf("\n\t----------------- 1170025    -   1171858   -   1170130------------------\n");
    printf("\n\t--------------------------------Randomly--------------------------------\n");
    printf("\n\t\t\t   initialy the seeker player is %d\n", seeker);
    printf("\n\t\t\t   initialy the thrower player is %d\n", thrower);
    printf("\n\n\t\t\tPlayers height are [%d:%d,%d:%d,%d:%d,%d:%d]\n", players[0].id, players[0].height, players[1].id, players[1].height, players[2].id, players[2].height, players[3].id, players[3].height);
    printf("%s\n\t\t\t  [player1:%d,player2:%d,player3:%d,player4:%d]%s\n", KYEL, players[0].num_seekers, players[1].num_seekers, players[2].num_seekers, players[3].num_seekers, KNRM);
    printf("\n\t---------------------------------------------------------------------------\n");
    printf("\n\n\n%s----------------------------------------Let the Game Begin!---------------------------------------%s\n\n\n", KBLU, KNRM);
    printf("Press Any Key to Continue\n\n");
    fflush(stdout);

    getchar();

    thr_id1 = pthread_create(&p_thread[0], NULL, player_code, (void *)&players[0]);
    thr_id2 = pthread_create(&p_thread[1], NULL, player_code, (void *)&players[1]);
    thr_id3 = pthread_create(&p_thread[2], NULL, player_code, (void *)&players[2]);
    thr_id4 = pthread_create(&p_thread[3], NULL, player_code, (void *)&players[3]);
    thr_id5 = pthread_create(&p_thread[4], NULL, referee_code, (void *)&num5);

    for (i = 0; i < 5; i++)
        pthread_join(p_thread[i], NULL);
    return 0;
}
