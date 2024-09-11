#include <iostream>
#include <fstream>
using  namespace std;
#define m 3
#define page_size  3

struct Record{
    int code;
    char name[20];
};

template<typename Tk>
struct indexNode{
    Tk key[m-1];
    Tk children[m];
    int count;
    int next_erased;


};

struct dataPage{
    Record records[page_size];
    int next_page;
    int next_erased;
    int count;

};


string indexfile = "index.dat";
string datafile = "data.dat";

template<typename Tk>
class BplusTree{
private:
    indexNode<Tk> * root;
    dataPage current;
    int metadata_size = int(sizeof(int));
    int index_fl_top; //Cabeza de puntero logico de la free list en indexfile
    int data_fl_top; //Cabeza de puntero logico de la free list en datafile
public:

    BplusTree(){
        create_new_files();
        lift_metadata();

    }

private:
    template<class T>
    int get_total_size(T generic_node ){


    }

    void create_new_files(){

        //Crear datafile si no existiera
        fstream file(datafile , ios::binary|ios::in|ios::out);

        if(!file.is_open())
        {
            file.open(datafile, ios::binary|ios::out);
            int default_free_list_top = -1;
            file.seekp(0, ios::beg);
            file.write((char*)(&default_free_list_top) , metadata_size);
            file.close();
        }

        file.close();

        //Crear indexfile si no existiera

        fstream file2(indexfile , ios::binary|ios::in|ios::out);

        if(!file2.is_open())
        {
            file2.open(indexfile, ios::binary|ios::out);
            int default_free_list_top = -1;
            file2.seekp(0, ios::beg);
            file2.write((char*)(&default_free_list_top) , metadata_size);
            file2.close();
        }

        file2.close();

    }
    void lift_metadata(){
        //Leer metadata de datafile
        fstream file(datafile , ios::binary|ios::in|ios::out);
        file.seekg(0 , ios::beg);
        file.read((char*)(&data_fl_top) , metadata_size);
        file.close();
        //Leer medatada de indexfile
        fstream file2(indexfile , ios::binary|ios::in|ios::out);
        file2.seekg(0 , ios::beg);
        file2.read((char*)(&index_fl_top) , metadata_size);
        file2.close();




    }
    void read_index_node_in_pos(fstream file , indexNode<Tk> &index_node , int pos)
    {



    }
    void write_index_node_in_pos(fstream& file ,indexNode<Tk> index_node , int pos){

    }

    void read_page_node_in_pos(fstream file  , dataPage &data_node , int pos){

    }
    void write_page_node_in_pos(fstream& file ,dataPage data_node , int pos){

    }






};
