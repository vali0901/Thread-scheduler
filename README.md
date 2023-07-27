# Thread Scheduler

Pentru acest scheduler au fost definite doua structuri de date:
* o coada de prioritati (in care sunt stocate noduri de tip thread_node
  ce descriu threaduri aflate in starea de Ready)
* o lista simplu inlantuita (in care sunt stocate elemente de tip thread_node
  semnificand threadurile aflate in starea Waiting)

Totodata, se va mai folosi un pointer la thread_node semnificand threadul
aflat in executie, un vector static care retine toate identificatoarele
(pthread_t) threadurilor si un contor ce numara cate threaduri sunt manipulate
de acest scheduler.

O functie esentiala este cea de reschedule(), care verifica daca threadul curent
trebuie schimbat, in caz afirmativ ocupandu-se de acest lucru. Aceasta functie
este apelata la finalul fiecarei functii (fork, exec, wait, signal, work).

Sincronizarea threadurilor a fost facuta cu ajutorul mutexurilor astfel:
un nou thread creat va incepe cu mutexul blocat; ulterior, el va intra in
functia work() unde se cere un lock pe mutexul sau; cand acest thread va fi
planificat si va fi noul thread Running, schedulerul va apela functia de mutex
unlock, permitandu-se astfel threadului sa continuie in functia work() si sa 
inceapa executia handlerului. Acelasi sistem se regaseste in fiecare functie
(so_fork, so_wait, so_exec si so_signal) pentru realizarea sincronizarii dintre
threaduri (permiterea numai unuia sa ruleze la un moment de timp).
Vulnerabilitatea acestei implementari vine din faptul ca functia de mutex_unlock
a unui thread este apelata din contextul altui thread, diferit (in unele cazuri)
de cel care a apelat mutex_lock, iar, conform manualului, aceasta actiune
provoaca un undefined behavior, o alternativa mai buna fiind probabil
implementarea cu semafoare.

Totodata este folosit un mutex pentru scheduler pentru a bloca intrarea in functia
so_end, aceasta fiind permisa doar atunci cand coada de prioritati ramane goala, 
iar niciun alt thread nu mai este in Runnning.

Functia de init aloca dinamic memorie pentru structura de coada si de lista,
functia de end dezaloca memoria alocata pentru coada si lista si da join pe
fiecare thread creat pentru a permite dezalocarea memoriei folosite de aceste
threaduri. De mentionat ca memoria folosita de fiecare structura de thread_node
este eliberata in functia work(), dupa ce executia handlerului se finalizeaza.

Functiile de fork, exec, wait si signal sunt descrise in comentariile codului. 