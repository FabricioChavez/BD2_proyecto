#include <iostream>
#include <fstream>
#include <tuple>
#include <algorithm>
#include <string.h>
using  namespace std;
#define m 3
#define page_size  3
template<typename Tk>
struct Record{
    Tk key;
    int code;
    char name[20];


    bool operator<(Record<Tk> other)
    {
        return key < other.key;
    }

};//24 size





template<typename Tk>
struct indexNode{
    Tk key[m-1];
    Tk children[m];
    int count;
    int next_erased;
    bool is_leaf;
    bool is_root;


};
//sizeof(Tk)*(2m-1) + 8
template<typename Tk>
struct dataPage{
    Record<Tk> records[page_size];
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
    dataPage<Tk> current;
    int metadata_size = int(sizeof(int));
    int index_length;
    int data_length;
    int index_fl_top; //Cabeza de puntero logico de la free list en indexfile
    int data_fl_top; //Cabeza de puntero logico de la free list en datafile
public:

    BplusTree(){
        index_length = sizeof(indexNode<Tk>);
        data_length = sizeof(dataPage<Tk>);
        create_new_files();//Create the new files in case they do not exist and write the metadata for freelist
        lift_metadata(); // Lift metadata if possible
    }


    void insert(Record<Tk> record  )
    {

    }

    Record<Tk>* search(Tk key)
    {

    }

    void remove(Tk key){

    }


private:

    tuple<dataPage<Tk> , dataPage<Tk> , int> split_leaf_node( dataPage<Tk> & to_split  , Tk key  , int index ){

        Record<Tk> r[page_size+1];
        dataPage<Tk> left_old;
        dataPage<Tk> right_new;
        int new_key;
        int mid = (page_size+1)/2;

        auto pos  = upper_bound(to_split.records , to_split.records+page_size , key) - &to_split.records[0];

        for (int i = 0; i < page_size+1 ; ++i) {

            if(i<pos)
                r[i]=to_split.records[i];

            if(i==pos)
                r[i]==key;
            if(i>pos)
                r[i]=to_split.records[i-1];

            if(i==mid) new_key ;

        }






    }

    tuple< indexNode<Tk> , indexNode<Tk>, int>  split_index_node(){

    }


    template<class T>
    void add_bucket_to_end(const string& filename   , T generic_bucket)
    {
        fstream file(filename , ios::binary | ios::in | ios::out);
        file.seekp(0 , ios::end);
        file.write((char*)(&generic_bucket) , sizeof(T));
    }


    template<class T>
    int get_total_size(fstream file, T generic_node ){
        file.seekg(0 , ios::end);
        int total = int(file.tellg()) - metadata_size;
        int object_size = sizeof(T);
        return total/object_size;
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

    void read_page_node_in_pos(fstream file  , dataPage<Tk> &data_node , int pos){
        file.seekg(metadata_size+ pos*data_length, ios::beg);
        file.read((char*)(&data_node) , data_length);

    }
    void write_page_node_in_pos(fstream& file ,dataPage<Tk> data_node , int pos){
        file.seekg(metadata_size+ pos*data_length, ios::beg);
        file.read((char*)(&data_node) , data_length);

    }




};

    void simulation( int &l , int& r  , int n)
    {
        if(n==0)
        {
            l = 5;
            r= 10 ;
            cout<<"end of recursion down "<<endl;
            return;
        }else {

            cout<<"De ida con n -->"<<n<<endl;
            cout<<"de ida con "<< l <<" y " <<r<<endl;
            simulation(l , r  , n-1);
        }


        //vuelta

        cout<<"return n-->"<<n<<endl;
        cout<<"de vuelta con "<< l <<" y " <<r<<endl;


    }

int main(){
        int l , r ;
        l=r=-1;
    simulation(l , r , 5);


}