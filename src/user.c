#include <pthread.h>
#include <errno.h>
#include "user.h"
#include <stdlib.h>
#include "clientthread.h"
#include <assert.h>
#include "util.h"
#include <string.h>
#include "serverconfig.h"
#include <unistd.h>


static pthread_mutex_t userLock = PTHREAD_MUTEX_INITIALIZER;
static User *userFront = NULL;
static User *userBack = NULL;


User *add_user(int socket,pthread_t thread,  char *uname, int do_lock){

  debugPrint("Adding new User %s",uname);
  User * new_user = malloc(sizeof(User));
  if(new_user == NULL){
    errorPrint("Out of memory!\nCould not create a new user!");
    return new_user;
  }
  memset(new_user,0,sizeof(User));
  new_user->thread = thread; 
  new_user-> sock = socket;
  strncpy(new_user-> name, uname, 31);
  new_user-> name[31]='\0';
  debugPrint(">>>> MUTEX ADD_USER");
  if(do_lock){
    lockMutex();
  }
  debugPrint("<<<< MUTEX ADD_USER");
  if(userFront == NULL && userBack == NULL){
    userFront = new_user;
    userBack = new_user;
  } else {
    new_user -> prev = userBack;
    userBack -> next = new_user;
    userBack = new_user;
  }
  if(do_lock){
    unlockMutex();
  }
  return new_user;
}

// Unused
void all_users(void(*func) (User *)){
  lockMutex();
  User *cUser = userFront;
  User *nUser = NULL;
  while(cUser != NULL){
    nUser = cUser->next;
    func(cUser);
    cUser = nUser;
  }
  unlockMutex();
  return;
}


int remove_user(User *user, int do_lock){
  debugPrint("[%li] user.c: remove_user called, removing user %s ", pthread_self(), user->name);
  User *cUser = userFront;
  debugPrint(">>> LOCKMUTEX_REM_USER");
  if(do_lock){
    lockMutex();
  }
  debugPrint("<<< LOCKMUTEX_REM_USER");
  while(cUser != NULL){ //iterating over all users
    if(strcmp(cUser->name, user->name) == 0){//deleting user
      debugPrint("[%li] user.c: Found user %s, removing...", pthread_self(), user->name);
      if(cUser->prev == NULL){
        userFront = cUser->next;
      } else {
        cUser -> prev -> next = cUser -> next;
      }
      if(cUser->next == NULL){
        userBack = cUser -> prev;
      } else {
        cUser -> next -> prev = cUser -> prev;
      }
      close(cUser->sock);
      debugPrint("[%li] user.c: Removed user %s ", pthread_self(), cUser->name);
      memset(cUser, 0, sizeof(User));
      free(cUser);
      if(do_lock){
        unlockMutex();
      }
      return 1;
    }
    cUser = cUser->next;
  }
  if(do_lock){
    unlockMutex();
  }
  debugPrint("[%li] user.c: User %s not found, so not removed", pthread_self(), user->name);
  return -1;
}


User *getFront(){
  debugPrint("[%li] user.c: getFront called.", pthread_self());
  debugPrint("Userprint: %li",(long)userFront);
  User *ret = userFront;
  if(ret != NULL){
    debugPrint("[%li] user.c: getFront returning User %s.", pthread_self(), ret->name);
  }
  else{
    debugPrint(" user.c: getFront returning NULL User Pointer.");
  }
  return ret;
}


int checkName(const char *name)
{

  if (strlen(name) > 31){//Name too long
    errno = EMSGSIZE;

    return -1;
  }
  for(int i =0; i< 31; i++)
  {
    if (name[i]=='\0') {
      break;
    }
    else if (name[i]<33||name[i]>126||name[i]==34||name[i]==39||name[i]==96)
    {//Name invalid
      errno = EINVAL;

      return -1;
    }
  }

  return 1; //Name valid
}


User *getUser(const char *username){
  lockMutex();
  User *aUser = userFront;
  while(aUser != NULL){
    if(strcmp(username, aUser->name) == 0){
      unlockMutex();
      return aUser;
    }
    aUser = aUser->next;
  }
  unlockMutex();
  return NULL;
}




int kick(const char *userName){
  // if user is admin DO NOT KICK!
  if(strcmp(serverconfig_getAdminName(), userName) == 0){
      errno = EILSEQ; // Illegal byte sequence C95 macro constant in Linux <errno.h>
      return -1;
  }
  User *kickedUser;

  // Does user exist?
  if((kickedUser = getUser(userName)) == NULL){
      errno = ENOENT; // Error NO ENTry
      return -1;
  }

  lockMutex();
  Message msg = createServerMessage("You have been kicked from the server.");
  networkSend(kickedUser->sock, &msg);
  // SOcket wird ind RemoveUser entfernt.

  // Noch ein Networksend. Warum genau? Keine Ahnung. Aber es scheint 
  // die einzige Möglichkeit zu sein, den Client sofort zu beenden.
  networkSend(kickedUser->sock, &msg);
  
  pthread_cancel(kickedUser->thread); // terminate thread
  pthread_join(kickedUser->thread, NULL); //await thread cancellation.
  remove_user(kickedUser, 0); // remove user from list
  unlockMutex();
  
  return 0;
}

int mutex = 0;
void lockMutex(){
  // Disable Cancellability During Mutex as this could lead to a permanent lock.
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
  pthread_mutex_lock(&userLock);
  mutex ++;
  debugPrint("mutex: %d +", mutex);
}
void unlockMutex(){
  pthread_mutex_unlock(&userLock);
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  mutex --;
  debugPrint("mutex: %d -", mutex);
}