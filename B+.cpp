#include <iostream>
#include <fstream>
#include <tuple>
#include <algorithm>
#include <vector>
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
    int next_removed;
    bool is_leaf;
    bool is_root;

    indexNode(){

        next_removed=-1;
        count=0;
    }


};
//sizeof(Tk)*(2m-1) + 8
template<typename Tk>
struct dataPage{
    Record<Tk> records[page_size];
    int next_page;
    int next_removed;
    int count;

    dataPage(){
        next_page=-1;
        next_removed=-1;
        count=0;
    }

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

    vector<Record<Tk>> range_search(Tk beg , Tk fin ){

    }



    void remove(Tk key){

    }


private:
    // Retorna left , right , key
    tuple<int, int , Tk> split_leaf_node( fstream &file ,int index,dataPage<Tk> & to_split  , Record<Tk> new_record   ){

        Record<Tk> r[page_size+1];
        dataPage<Tk> left_old;
        dataPage<Tk> right_new;
        Tk new_key;
        int mid = (page_size+1)/2;

        //calcular posicion donde debe introducirce la key ficticia para hacer el split
        auto pos  = upper_bound(to_split.records , to_split.records+page_size , new_record) - &to_split.records[0];

        //hacer el split de los nodos
        for (int i = 0; i < page_size ; ++i) {

            if(i<pos)
            {
                r[i]=to_split.records[i];
                left_old.records[left_old.count++]=r[i];

            }

            if(i==pos)
            {
                r[i]=new_record.key;
                left_old.records[left_old.count++]=r[i];
            }
            if(i>pos)
            {
                r[i+1]=to_split.records[i];
                right_new.records[right_new.count++]=r[i+1];
            }

            if(i==mid) new_key =  r[i].key ;

        }


        int next_logical_index = calculate_index();  //calcuar pos para ubicar el nuevo page bucket generado
        left_old.next_page =next_logical_index;  //  asignar el valor del index nuevo a la hoja izquierda para enlazarlos
        write_page_node_in_pos(file , left_old , index); // escribir el left en la posicon original
        write_page_node_in_pos(file , new_record , next_logical_index); // escribir el right en la nueva posicion

        tuple<int , int , int> lrk = {index , next_logical_index , new_key};

        return lrk;

    }

    tuple< int , int, Tk>  split_index_node(fstream &file ,int index,indexNode<Tk> & to_split  , Tk key  ){
        Tk rkey[m];
        Tk rchildren[m+1];
        indexNode<Tk> left;
        indexNode<Tk> right;

    }

    template<class T>
    int calculate_index(){
        return -1;
    }

    template<dataPage<Tk>>
    int calculate_index(){ //calculate next logical index or assign you a free space from the free list
        fstream file(datafile , ios::in| ios::out);
        dataPage<Tk> b ;

        if(data_fl_top != -1)
        {   int ans = data_fl_top;
            b = read_page_node_in_pos(file , data_fl_top );
            data_fl_top = b.next_removed;
            file.seekp(0,ios::beg);
            file.write((char*)(&data_fl_top), metadata_size);
            return ans;
        }

        file.seekg(0 , ios::end);
        int total = int(file.tellg())-metadata_size/sizeof(dataPage<Tk>);
        return total+1;

    }

    template<indexNode<Tk>>
    int calculate_index(){

    }


    template<class T>
    void add_bucket_to_end(const string& filename   )
    {
        T generic_bucket;
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

    dataPage<Tk> read_page_node_in_pos(fstream file  , int pos){
        dataPage<Tk> data_node;
        file.seekg(metadata_size+ pos*data_length, ios::beg);
        file.read((char*)(&data_node) , data_length);
        return data_node;

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