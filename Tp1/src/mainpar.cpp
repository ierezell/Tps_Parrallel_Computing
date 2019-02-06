#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <gmp.h>
#include <typeinfo>
#include <string>

using namespace std;

void* compute_interval(void* arg){
    //executed by threads
}

int main(int argc, char *argv[])
{
    // Check for correct usage
    if (argc <= 2 || argc > 3)
    {
        cout << "Usage : " << argv[0] << "<nb_threads> <fichier.txt>.\n";
        return EXIT_FAILURE;
    }
    int nb_thread = atoi(argv[1]);
    int number_of_lines = 0;
    typedef mpz_t intervalle[2];
    typedef vector<intervalle> list_thread_intervalle;
    typedef list_thread_intervalle list_total_intervalle[nb_thread];
    // Check is file for numbers interval is correct
    ifstream prime_nb_file;
    prime_nb_file.open(argv[2]);
    string line;
    if (prime_nb_file.is_open() == 0)
    {
        cout << "Impossible d'ouvrir le fichier.\n";
        return EXIT_FAILURE;
    }
    
    // Divide the file in nb_thread buffers for the threads
    while (getline(prime_nb_file, line))
        number_of_lines++;
    cout << "number_of_lines" << number_of_lines << endl;
    int i = 0;

    list_total_intervalle lTIntervalles;
    while (getline(prime_nb_file, line))
    {
        intervalle buffer;
        gmp_sscanf(line.c_str(), "%Zd %Zd", buffer[0], buffer[1]);
        //list_total_intervalle[i % nb_thread].push_back(buffer);
        i++;
    }
    //create threads   
    pthread_t thread_Ids[nb_thread];
    for (int i = 0; i < nb_thread; i++)
    {
        pthread_create(&thread_Ids[i], NULL, compute_interval,(void*)NULL );
    }
}


