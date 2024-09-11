#include <iostream>
#include <fstream>
using  namespace std;
#define m 3
#define page_size  3

struct Record{
    int code;
    char name[20];
};//24 size

template<typename Tk>
struct indexNode{
    Tk key[m-1];
    Tk children[m];
    int count;
    int next_erased;


};
//sizeof(Tk)*(2m-1) + 8
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
    int index_length;
    int data_length;
    int index_fl_top; //Cabeza de puntero logico de la free list en indexfile
    int data_fl_top; //Cabeza de puntero logico de la free list en datafile
public:

    BplusTree(){
        index_length = sizeof(indexNode<Tk>);
        data_length = sizeof(dataPage);
        create_new_files();//Create the new files in case they do not exist and write the metadata for freelist
        lift_metadata(); // Lift metadata if possible
    }

private:
    template<class T>
    int get_total_size(fstream file, T generic_node ){
        file.seekg(0 , ios::end);
        int total = int(file.tellg()) - metadata_size;
        int object_size = sizeof(T);
        return total/object_size;\
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
        file.seekg(metadata_size+ pos*index_length, ios::beg);
        file.read((char*)(&index_node) , index_length);

    }
    void write_index_node_in_pos(fstream& file ,const indexNode<Tk> index_node , int pos){

        file.seekp(metadata_size + pos*index_length , ios::beg);
        file.write((char*)(&index_node) , index_length);

    }

    void read_page_node_in_pos(fstream file  , dataPage &data_node , int pos){
        file.seekg(metadata_size+ pos*data_length, ios::beg);
        file.read((char*)(&data_node) , data_length);

    }
    void write_page_node_in_pos(fstream& file ,dataPage data_node , int pos){
        file.seekg(metadata_size+ pos*data_length, ios::beg);
        file.read((char*)(&data_node) , data_length);

    }


};


int main(){




}