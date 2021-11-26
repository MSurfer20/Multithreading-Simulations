#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#define COLOR_BLACK   "\033[0;30m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_YELLOW  "\033[0;33m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_MAGENTA "\033[0;35m"
#define COLOR_CYAN    "\033[0;36m"
#define COLOR_WHITE   "\033[0;37m"
#define COLOR_RESET   "\033[0m"

#define COLOR_GREEN_BOLD   "\033[1;32m"
#define COLOR_RED_BOLD     "\033[1;31m"
#define TEXT_UNDERLINE     "\033[4m"
#define TEXT_BOLD          "\033[1m"

int num_students, num_labs, num_courses;
struct course
{
    int id;
    char name[1000];
    double interest;
    int course_max_slots;
    int num_labs;
    int lab_ids[1000];
    bool registration_open;
    pthread_mutex_t course_mutex;

    pthread_cond_t tut_slots_condn;
    pthread_cond_t tut_session_condn;
    int num_slots;
};
struct course* courses[1000];

struct student
{
    int id;
    double calibre;
    int course_1;
    int course_2;
    int course_3;
    double wait_time;
};
struct student* students[1000];

struct lab
{
    int id;
    char name[1000];
    int num_tas;
    int tut_limit;
    int ta_tut_count[1000];
    bool ta_free[1000];
    int remaining_tas; //Not used, only init

    int curr_acceptable_tut_count; //Current max for vonus
    int remaining_curr_count; //Number of TAs who can allotted in accordance with bonus
    pthread_mutex_t lab_mutex;

    pthread_mutex_t remaining_tas_mutex;
};
struct lab* labs_array[1000];

pthread_t course_th[1000];
pthread_t student_th[1000];
pthread_mutex_t ta_mutex[1000][1000];

int random_no_range(int a, int b)
{
    int c = (rand()%(b-a+1)) + a;
    return c;
}

void *course_thread(void* arg)
{
    struct course* cour=(struct course*)arg;
    while(cour->registration_open)
    {
        sleep(2);
        int chosen_ta_id = -1, chosen_ta_lab=-1, can_find_ta=0, tut_slots=0;
        pthread_mutex_lock(&cour->course_mutex);
        cour->num_slots=0;
        pthread_mutex_unlock(&cour->course_mutex);
        for(int lab_no=0;lab_no<cour->num_labs; lab_no++)
        {
            if(chosen_ta_id!=-1)
                break;
            struct lab* la = labs_array[cour->lab_ids[lab_no]];
            // printf("+++++++++++++++++++++++++++++++++COURSE %s IS LOOKING FOR TA IN %s LAB\n", cour->name, la->name);
            int lab_id=cour->lab_ids[lab_no];
            for(int ta_no=0;ta_no<la->num_tas;ta_no++)
            {
                pthread_mutex_lock(&ta_mutex[lab_id][ta_no]);
                if(la->ta_tut_count[ta_no] < la->tut_limit)
                    can_find_ta=1;
                // printf("++++++++++++++++++++++++++++++TA_FREE: %d, TA_TUT_COUNT: %d, TUT_LIMIT: %d, CURR_ACCEPTABLE_COUNT: %d\n", la->ta_free[ta_no], la->ta_tut_count[ta_no], la->tut_limit, la->curr_acceptable_tut_count);
                // if((!la->ta_free[ta_no]) || la->ta_tut_count[ta_no] >= la->tut_limit)
                if((!la->ta_free[ta_no]) || la->ta_tut_count[ta_no] >= la->tut_limit || la->ta_tut_count[ta_no]!=la->curr_acceptable_tut_count)
                {
                    pthread_mutex_unlock(&ta_mutex[lab_id][ta_no]);
                    continue;
                }
                chosen_ta_id=ta_no;
                chosen_ta_lab=lab_id;
                la->ta_free[ta_no]=false;
                la->ta_tut_count[chosen_ta_id]++;
                pthread_mutex_lock(&la->lab_mutex);
                la->remaining_curr_count--;
                if(la->remaining_curr_count==0)
                    la->curr_acceptable_tut_count++, la->remaining_curr_count=la->num_tas;
                pthread_mutex_unlock(&la->lab_mutex);
                pthread_mutex_unlock(&ta_mutex[lab_id][ta_no]);
                break;
            }
        }

        pthread_mutex_lock(&cour->course_mutex);
        if(chosen_ta_id==-1)
        {
            if(can_find_ta==0)
            {
                cour->registration_open=false;
                printf(COLOR_RED_BOLD "Course %s doesn’t have any TA’s eligible and is removed from course offerings\n"COLOR_RESET, cour->name);
                pthread_cond_broadcast(&cour->tut_slots_condn);
                pthread_cond_broadcast(&cour->tut_session_condn);
            }
            pthread_mutex_unlock(&cour->course_mutex);
            continue;
        }
        struct lab* la = labs_array[chosen_ta_lab];
        printf(COLOR_GREEN_BOLD "TA %d from lab %s has been allocated to course %s for %dth TA ship\n"COLOR_RESET, chosen_ta_id, labs_array[chosen_ta_lab]->name, cour->name, la->ta_tut_count[chosen_ta_id]);

        int num_slots = random_no_range(1, cour->course_max_slots);
        cour->num_slots=num_slots;
        int og_seats=cour->num_slots;
        printf(COLOR_BLUE"Course %s has been allocated %d seats\n"COLOR_RESET, cour->name, cour->num_slots);

        pthread_cond_broadcast(&cour->tut_slots_condn);
        pthread_mutex_unlock(&cour->course_mutex);

        sleep(2);

        pthread_mutex_lock(&cour->course_mutex);

        printf(COLOR_YELLOW"Tutorial has started for Course %s with %d seats filled out of %d\n"COLOR_RESET, cour->name, og_seats-cour->num_slots, og_seats);
        cour->num_slots=0;
        sleep(6);
        pthread_cond_broadcast(&cour->tut_session_condn);
        pthread_mutex_unlock(&cour->course_mutex);

        pthread_mutex_lock(&ta_mutex[chosen_ta_lab][chosen_ta_id]);
        la->ta_free[chosen_ta_id]=true;
        pthread_mutex_unlock(&ta_mutex[chosen_ta_lab][chosen_ta_id]);
        printf(COLOR_CYAN"TA %d from lab %s has completed the tutorial and left the course %s\n"COLOR_RESET, chosen_ta_id, labs_array[chosen_ta_lab]->name, cour->name);
        if(la->ta_tut_count[chosen_ta_id] == la->tut_limit)
        {
            pthread_mutex_lock(&la->remaining_tas_mutex);
            la->remaining_tas--;
            if(la->remaining_tas==0)
            {
                printf(COLOR_CYAN TEXT_UNDERLINE "Lab %s no longer has students available for TA ship\n" COLOR_RESET, la->name);
                la->remaining_tas=-1;   
            }
            pthread_mutex_unlock(&la->remaining_tas_mutex);
        }
    }

    return NULL;
}


void *student_thread(void* arg)
{
    struct student* stud=(struct student*)arg;

    sleep(stud->wait_time);

    printf(COLOR_GREEN_BOLD"Student %d has filled in preferences for course registration\n"COLOR_RESET, stud->id);

    for(int pref_num=0; pref_num<3; pref_num++)
    {
        int course_id=0;
        if(pref_num==0)
            course_id=stud->course_1;
        else if(pref_num==1)
        {
            course_id=stud->course_2;
            int prev_course_id=stud->course_1;
            printf(TEXT_UNDERLINE COLOR_RED"Student %d has changed current preference from %s to %s\n"COLOR_RESET, stud->id, courses[prev_course_id]->name, courses[course_id]->name);
        }
        else
        {
            course_id=stud->course_3;
            int prev_course_id=stud->course_2;
            printf(TEXT_UNDERLINE COLOR_RED"Student %d has changed current preference from %s to %s\n"COLOR_RESET, stud->id, courses[prev_course_id]->name, courses[course_id]->name);
        }
        struct course* cour = courses[course_id];

        pthread_mutex_lock(&cour->course_mutex);

        while(cour->registration_open && cour->num_slots==0)
        {
            pthread_cond_wait(&cour->tut_slots_condn, &cour->course_mutex);
        }

        if(!cour->registration_open)
        {
            pthread_mutex_unlock(&cour->course_mutex);
            continue;
        }

        cour->num_slots--;
        printf(TEXT_UNDERLINE COLOR_GREEN_BOLD"Student %d has been allocated a seat in course %s\n"COLOR_RESET, stud->id, cour->name);

        pthread_cond_wait(&cour->tut_session_condn, &cour->course_mutex);
        pthread_mutex_unlock(&cour->course_mutex);

        double prob = stud->calibre * cour->interest;
        double random_prob = (double)rand() / (double)RAND_MAX;

        if(random_prob<prob)
        {
            printf(TEXT_UNDERLINE COLOR_BLUE"Student %d has selected course %s permanently\n"COLOR_RESET, stud->id, cour->name);
            return NULL;
        }
        else
        {
            printf(COLOR_MAGENTA"Student %d has withdrawn from course %s\n"COLOR_RESET, stud->id, cour->name);
        }
    }
    printf(COLOR_RED_BOLD TEXT_UNDERLINE"Student %d couldn't get any of the courses.\n"COLOR_RESET, stud->id);
    return NULL;
}



int main()
{
    scanf("%d %d %d", &num_students, &num_labs, &num_courses);
    // printf("AAAAAAAAAAAAAAAAAAAAAAAAa");
    for(int x=0;x<num_courses;x++)
    {
        courses[x]=calloc(1, sizeof(struct course));
        scanf("%s %lf %d %d", courses[x]->name, &courses[x]->interest, &courses[x]->course_max_slots, &courses[x]->num_labs);
        // printf("BBBBBBBBBBBBBBBB");
        courses[x]->registration_open=true;
        pthread_mutex_init(&courses[x]->course_mutex, NULL);
        pthread_cond_init(&courses[x]->tut_session_condn, NULL);
        pthread_cond_init(&courses[x]->tut_slots_condn, NULL);
        for(int y=0;y<courses[x]->num_labs;y++)
            scanf("%d", &courses[x]->lab_ids[y]);
        courses[x]->id=x;
        courses[x]->num_slots=0;
    }
    // printf("AAAAAAAAAAAAAAAAAAAAAAAAa");

    for(int x=0;x<num_students;x++)
    {
        students[x]=calloc(1, sizeof(struct student));
        scanf("%lf %d %d %d %lf", &students[x]->calibre, &students[x]->course_1, &students[x]->course_2, &students[x]->course_3, &students[x]->wait_time);
        students[x]->id=x;
    }
    // printf("AAAAAAAAAAAAAAAAAAAAAAAAa");

    for(int x=0;x<num_labs;x++)
    {
        labs_array[x]=calloc(1, sizeof(struct lab));
        scanf("%s %d %d", labs_array[x]->name, &labs_array[x]->num_tas, &labs_array[x]->tut_limit);
        for(int y=0;y<labs_array[x]->num_tas;y++)
        {
            pthread_mutex_init(&ta_mutex[x][y], NULL);
            labs_array[x]->ta_free[y]=true;
            labs_array[x]->ta_tut_count[y]=0;
        }
        labs_array[x]->remaining_tas=labs_array[x]->num_tas;
        labs_array[x]->curr_acceptable_tut_count=0;
        labs_array[x]->remaining_curr_count=labs_array[x]->num_tas;

        pthread_mutex_init(&labs_array[x]->lab_mutex, NULL);
        pthread_mutex_init(&labs_array[x]->remaining_tas_mutex, NULL);
        labs_array[x]->id=x;
    }
    // printf("AAAAAAAAAAAAAAAAAAAAAAAAa");

    for(int x=0;x<num_courses;x++)
    {
        struct course* ind=(struct course*)malloc(sizeof(struct course));
        ind=courses[x];
        pthread_create(&course_th[x], NULL, course_thread, ind);
    }
    // printf("AAAAAAAAAAAAAAAAAAAAAAAAa");

    for(int x=0;x<num_students;x++)
    {
        struct student* ind=(struct student*)malloc(sizeof(struct student));
        ind=students[x];
        pthread_create(&student_th[x], NULL, student_thread, ind);
    }
    // printf("AAAAAAAAAAAAAAAAAAAAAAAAa");


    for(int x=0;x<num_students;x++)
    {
        pthread_join(student_th[x], NULL);
    }

    printf("SIMULATION OVER->\n");
    exit(0);
}
