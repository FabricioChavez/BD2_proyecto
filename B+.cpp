#include <iostream>
#include <fstream>
#include <tuple>
#include <algorithm>
#include <vector>
#include <string.h>
#include <iomanip>
using  namespace std;
#define m 3
#define page_size  3
template<typename Tk>
struct Record{
    Tk key;
    int code;
    char name[20];

    Record(){

    }

    Record(Tk key, int code, string name_) : key(key), code(code){
        strcpy(name , name_.c_str());
    }


    bool operator<(Record<Tk> other)
    {
        return key < other.key;
    }

    bool operator>(Record<Tk> other)
    {
        return key > other.key;
    }

    bool operator==(Record<Tk> other)
    {
        return key == other.key;
    }



};//24 size
template<typename Tk>
std::ostream& operator<<(std::ostream& os, const Record<Tk>& record) {
    os << "Key: " << record.key << ", Code: " << record.code << ", Name: " << record.name;
    return os;
}


template<typename Tk>
struct indexNode{
    Tk key[m-1];
    int children[m];
    int count;
    int next_removed;
    bool is_last;
    bool is_root;

    indexNode(){

        next_removed=-1;
        count=0;
        is_last= false;
        is_root = false;
    }


    void showdata()
    {
        cout<<"-------------Index data-------------"<<endl;
        cout<<"Root status  :"<<boolalpha<<is_root<<endl;
        cout<<"Last status  :"<<boolalpha<<is_last<<endl;
        cout<<"Count        :"<<count<<endl;
        cout<<"Next removed :"<<next_removed<<endl;
        cout<<">>>>>>>>Keys >>>>>>>>>>>>>>>>>>>>>>"<<endl;
        for (int i = 0; i <count ; ++i) {
            cout<<setw(10)<<key[i]<<" ";
        }
        cout<<">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"<<endl;
        cout<<"<<<<<<<<Children pointer<<<<<<<<<<<"<<endl;

        if(count>0)
        {
            for (int i = 0; i <count+1 ; ++i) {
                cout<<setw(10)<<children[i]<<" ";
            }
        }

        cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"<<endl;


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

    void showdata()
    {
        cout<<"-------------Page data-------------"<<endl;
        cout<<"next_page      :"<<next_page<<endl;
        cout<<"next_removed   :"<<next_removed<<endl;
        cout<<"Count          :"<<count<<endl;
        cout<<"<<<<<<<<RECORDS<<<<<<<<<<<"<<endl;
        for (int i = 0; i <count ; ++i) {
            cout<<records[i]<<endl;
        }
        cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"<<endl;


    }

};


string indexfile = "index.bin";
string datafile = "data.bin";

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

        print_all();

        //        lift_metadata(); // Lift metadata if possible
    }


    void insert(Record<Tk> record )
    {

        fstream file_index(indexfile , ios::binary | ios::in|ios::out);
        fstream file_data(datafile , ios::binary |ios::in | ios::out);

        insert(file_index , file_data , record , 0 , -1 ,-1 ,Tk{} , false );

        file_index.close();
        file_data.close();



    }


    void insert(fstream& file_index , fstream  & file_data, Record<Tk>& record,int logical_pointer , int &left_up , int& right_up , Tk &up_key , bool &there_is_excess  )
    {
        indexNode<Tk> node = read_index_node_in_pos(file_index , logical_pointer);
        if(node.is_last)
        {

            if(node.count == 0) // Si realmente aun no hay nada en root solo se inserta directamente
            {

                if(insert_in_leaf(file_data ,0 , record)) {
                    return;
                }else {
                    //Primer split
                    dataPage<Tk> b = read_page_node_in_pos(file_data , 0);
                    tuple<int , int , Tk> lrk = split_leaf_node(datafile , 0, b , record  );
                    //Dos nuevos primeros nodos generados
                    int left_leaf_pointer = get<0>(lrk);
                    int right_leaf_pointer =get<1>(lrk);
                    Tk split_key = get<2>(lrk);

                    //Insertar ordenadamente , Al ser la primera vez no se tienen en cuenta un posible spliteo
                    insert_in_index(file_index , 0 , split_key , left_leaf_pointer  , right_leaf_pointer);
                    return;
                }

            }


            int pos =  int(upper_bound(node.key  , node.key + size , record.key) - &node.key[0]) ;

            if(insert_in_leaf(file_data , pos, record)) {
                return;
            }else {
                //Si no hay espacio se splitea la hoja y luego se inserta esto en el index
                dataPage<Tk> b = read_page_node_in_pos(file_data , pos);
                tuple<int , int , Tk> lrk = split_leaf_node(datafile , pos, b , record  );
                //Dos nuevos primeros nodos generados
                int left_leaf_pointer = get<0>(lrk);
                int right_leaf_pointer =get<1>(lrk);
                Tk split_key = get<2>(lrk);


                //insertar ordenadamente la nueva key y ubicar a sus hijos
                if(insert_in_index(file_index , logical_pointer , split_key , left_leaf_pointer , right_leaf_pointer))
                {
                    //si se inserta si problemas en el nodo solo retornas tranquilamente
                    return;
                }else {
                    //Si este nodo es la raiz y a la vez sabemos que es el last entones es la primera vez q sube esta propiedad de raiz y hace crecer el arbol
                    if(node.is_root)
                    {

                        indexNode<Tk> new_root;
                        new_root.is_root =true;
                        new_root.is_last = false;
                        //splitear el nuevo index (las hojas se asignaran dentro ordenadamente a cada mitad correspondiente)
                        tuple<int , int , Tk>  lrk_index = split_index_node(file_index ,logical_pointer,node , split_key , left_leaf_pointer , right_leaf_pointer);
                        int left_index = get<0>(lrk_index);
                        int right_index = get<1>(lrk_index);
                        Tk new_root_key = get<2>(lrk_index);

                        //calculuar un nuevo index logico este sera el nuevo left
                        int new_logical_index = calculate_index(node);
                        //Extraer el puntero izquierdo de su posicion
                        indexNode<Tk> old_left = read_index_node_in_pos(file_index , left_index);
                        //escribir left en la nueva posicion
                        write_index_node_in_pos(file_index,old_left , new_logical_index);
                        //escribir  root en la poscion cero
                        new_root.children[new_root.count]=new_logical_index;//se asigna nodo de la izquierda
                        new_root.children[++new_root.count]=right_index;//se asigna nodo de la derehca
                        new_root.key[new_root.count]=new_root_key; //asigna la primera key del split
                        //escribir la raiz --> siempre posicion cero
                        write_page_node_in_pos(file_index , new_root, 0);
                        //retornar
                        return;

                    }


                    // En caso de que no sea root solo se splitea y se manda hacia arriba los valores a insertar en el nivel superior (todo lo demas sera manegada por la vuelta de la recursion)

                    //splitear el nuevo index (las hojas se asignaran dentro ordenadamente a cada mitad correspondiente)
                    tuple<int , int , Tk>  lrk_index = split_index_node(file_index ,logical_pointer,node , split_key , left_leaf_pointer , right_leaf_pointer);
                    //Se asignan los valores que subiran en la vuelta de la recursion
                    left_up = get<0>(lrk_index);
                    right_up = get<1>(lrk_index);
                    up_key = get<2>(lrk_index);
                    //Con esto se indica a los niveles superiores en la vuelta de la recursion que existe algo que insertar
                    there_is_excess= true;
                    //Retornarn
                    return;

                }


            }


        }else{
                //next index te da el indice logico q esta en el child no se use para obtener donde insertar algo
            int next_index = get_next_index(node.key ,  node.count, record.key  , node.children );
            insert(file_index , file_data , record , next_index , left_up , right_up , up_key  , there_is_excess);

        }


        //Vuelta de la recursion donde los milagros pasan ;3

        if(there_is_excess)
        {

            //Si no es root no se hace nada fundamentalmetne diferente , ya que se asume que suben los valores
            if(insert_in_index(file_index ,  logical_pointer , up_key , left_up , right_up ))
            {
                //si se inserta y no hay problema solo se pone este flag en falso y ya
                there_is_excess= false;
            }else{


                //Si el nodo que se tiene que splitear es root se diverge un poco
                if(node.is_root){

                    indexNode<Tk> new_root;
                    new_root.is_root =true;
                    new_root.is_last = false;
                    //splitear el nuevo index (las hojas se asignaran dentro ordenadamente a cada mitad correspondiente)
                    tuple<int , int , Tk>  lrk_index = split_index_node(file_index ,logical_pointer,node , up_key ,left_up , right_up);
                    int left_index = get<0>(lrk_index);
                    int right_index = get<1>(lrk_index);
                    Tk new_root_key = get<2>(lrk_index);

                    //calculuar un nuevo index logico este sera el nuevo left
                    int new_logical_index = calculate_index(node);
                    //Extraer el puntero izquierdo de su posicion
                    indexNode<Tk> old_left = read_index_node_in_pos(file_index , left_index);
                    //escribir left en la nueva posicion
                    write_index_node_in_pos(file_index,old_left , new_logical_index);
                    //escribir  root en la poscion cero
                    new_root.children[new_root.count]=new_logical_index;//se asigna nodo de la izquierda
                    new_root.children[++new_root.count]=right_index;//se asigna nodo de la derehca
                    new_root.key[new_root.count]=new_root_key; //asigna la primera key del split
                    //escribir la raiz --> siempre posicion cero
                    write_page_node_in_pos(file_index , new_root, 0);
                    there_is_excess = false;

                }else {

                    //splitear el nuevo index (las hojas se asignaran dentro ordenadamente a cada mitad correspondiente)
                    tuple<int , int , Tk>  lrk_index = split_index_node(file_index ,logical_pointer,node , up_key , left_up , right_up);
                    //Se asignan los valores que subiran en la vuelta de la recursion
                    left_up = get<0>(lrk_index);
                    right_up = get<1>(lrk_index);
                    up_key = get<2>(lrk_index);


                    //Con esto se indica a los niveles superiores en la vuelta de la recursion que existe algo que insertar
                    there_is_excess= true;
                    //Retornarn

                }

            }
        }




    }




    Record<Tk>* search(Tk key)
    {

    }

    vector<Record<Tk>> range_search(Tk beg , Tk fin ){

    }



    void remove(Tk key){

    }


    void print_all(){

        indexNode<Tk> index_node;
        dataPage<Tk> data_node;

        fstream file_index(indexfile , ios::binary | ios::in|ios::out);
        fstream file_data(datafile , ios::binary |ios::in | ios::out);
        int index_total = get_total_size<indexNode<Tk>>(file_index );
        int total_pages = get_total_size<dataPage<Tk>>(file_data);
        cout<<"-------------INDEX NODES BEGIN ---------------------------"<<endl;

        file_index.seekg(0 , ios::beg);
        file_data.seekg(0, ios::beg);

        for (int i = 0; i < index_total; ++i) {
            index_node = read_index_node_in_pos(file_index , i);
            cout<<"------- Node number ["<<i<<"]"<<"-----------"<<endl;
            index_node.showdata();
            cout<<"---------------------------------------------"<<endl;

        }

        cout<<"-------------INDEX NODES END---------------------------"<<endl;

        cout<<"-------------DATA NODES BEGIN ---------------------------"<<endl;
        for (int i = 0; i < total_pages; ++i) {
            data_node = read_page_node_in_pos(file_data , i);
            cout<<"------- Node number ["<<i<<"]"<<"-----------"<<endl;
            data_node.showdata();
            cout<<"---------------------------------------------"<<endl;

        }
        cout<<"-------------DATA NODES END -----------------------------"<<endl;



    }


private:

    //funciones de insercion
    //inserta ordenadamente en hoja
    bool insert_in_leaf(fstream & file, int index  , Record<Tk>& record){


        dataPage<Tk> to_insert = read_page_node_in_pos(file , index);
        //Si la insercion no es posible retorna false (esto luego se maneja en un split)
        if(to_insert.count+1>page_size) return false;

        if(to_insert.count ==0){
            to_insert.records[to_insert.count++]=record;
            return true;
        }
        //encuentra la poscion del primer elemento mayor al que se inserta
        auto pos  = upper_bound(to_insert.records , to_insert.records+page_size , record) - &to_insert.records[0];

        //inserta ordenadamente en la posicon
        for (int i = ++to_insert.count; i >=pos ; --i) {\
            if(i==pos)
                to_insert[i]=record;
            if(i>pos)
                to_insert[i]=to_insert[i-1];
        }

        //Escribir el valor del bucker actualizado en memoria
        write_page_node_in_pos(file , to_insert , index); //
        //retorna true una vez completado el proceso
        return true;

    }

    //inserta ordenadamente en nodo interno
    bool insert_in_index(fstream & file ,   int index , Tk new_key ,  int left_pointer , int right_pointer ){

        indexNode<Tk> to_insert = read_index_node_in_pos(file , index);
        //Si se llena con esto se mandaria a un split en vez de insertarse se manda falso por q se viola la regla
        if(to_insert.count+1>m-1) return false;

        //Si se trata del primer insert solo se pone en la primera posicion

        if(to_insert.count==0)
        {   to_insert.children[to_insert.count]=left_pointer;
            to_insert.children[to_insert.count]= right_pointer;
            to_insert.key[to_insert.count++]=new_key;
            return true;
        }

        //encuentra la poscion adecuada de la key

        int pos = int(upper_bound(to_insert.key , to_insert.key+to_insert.count , new_key) -&to_insert.key[0]);
        //Ubicar la key y los nodos hijos en la posicon adecuada
        for (int i = ++to_insert.count+1; i>pos  ; --i) {

            if(i==pos)
            {
                to_insert.key[i] = new_key;
                to_insert.children[pos]=left_pointer;
                to_insert.children[pos+1]=right_pointer;
            }


            if(i > pos and i <= to_insert.count)  to_insert.key[i]=to_insert.key[i-1];

            if(i>pos+1) to_insert.children[i]=to_insert.children[i-1];

        }
        //Se escribe el nodo en la posicion indicada
        write_index_node_in_pos(file , to_insert , index);

        //retornar verdadero una vez concluye el proceso
        return true;

    }




    // Retorna left , right , key
    tuple<int, int , Tk> split_leaf_node( fstream &file ,int index,dataPage<Tk> & to_split  , Record<Tk> new_record   ){

        Record<Tk> r[page_size+1];
        dataPage<Tk> left_old;
        dataPage<Tk> right_new;
        Tk new_key;
        int mid = (page_size+1)/2;

        //calcular posicion donde debe introducirce la key ficticia para hacer el split
        auto pos  = upper_bound(to_split.records , to_split.records+page_size , new_record) - &to_split.records[0];

        //Pasar todos los nodos a un lugar con un lugar mas para posicionarlos
        for (int i = 0; i < page_size +1 ; ++i) {

            if(i<pos)
                r[i]=to_split.records[i];


            if(i==pos)
                r[i]=new_record.key;

            if(i>pos)
                r[i]=to_split.records[i-1];

            if(i==mid) new_key =  r[i].key ;

        }

        //Hacer el split de los nodos
        for (int i = 0; i < page_size+1; ++i) {

            if(i< mid)
                left_old.records[left_old.count++] = r[i];
            if(i>=mid)
                right_new.records[right_new.count++] =r[i];
        }
        dataPage<Tk> dummy;//variable para activar sobrecarga
        int next_logical_index = calculate_index(dummy);  //calcuar pos para ubicar el nuevo page bucket generado
        right_new.next_page = to_split.next_page; // Asignar puntero a la derecha que el nodo original apuntaba
        left_old.next_page =next_logical_index;  //  asignar el valor del index nuevo a la hoja izquierda para enlazarlos
        write_page_node_in_pos(file , left_old , index); // escribir el left en la posicon original
        write_page_node_in_pos(file , new_record , next_logical_index); // escribir el right en la nueva posicion

        tuple<int , int , Tk> lrk = {index , next_logical_index , new_key};

        return lrk;

    }

    tuple< int , int, Tk>  split_index_node(fstream &file ,int index,indexNode<Tk> & to_split  , Tk key  , int new_child_l , int new_child_right  ){
        Tk rkey[m];
        Tk rchildren[m+1];
        indexNode<Tk> left;
        indexNode<Tk> right;
        left.is_last = right.is_last = to_split.is_last; // Si se da el caso de que el nodo es lasts se pasa esa propiedad si no lo es no se pasa la propiedad

        Tk new_key;
        int mid = (m)/2;
        auto pos = upper_bound(to_split.key , to_split.key + m-1 , key)  - &to_split.key[0];

        //Reorganizar nodos internos
        for (int i = 0; i < m+1; ++i) {

            if(i < pos )
            {
                if(i<m)
                rkey[i]=to_split.key[i];

                rchildren[i]=to_split.children[i];
            }

            if(i == pos){
                if(i<m)
                    rkey[i]==key;

                rchildren[i]= new_child_l;
                rchildren[i+1]= new_child_right;
            }


            if(i > pos)
            {

                if(i<m)
                    rkey[i] = to_split.key[i-1];

                if(i>pos+1)
                rchildren[i]=to_split.children[i-1];
            }


            if(i==mid) new_key = rkey[i];
        }

        //Splitear nodo interno
        int i=0;
        for ( ; i < mid ; ++i) {
            left.children[left.count]=rchildren[i];
            left.key[left.count++]=rkey[i];

        }

        left.children[left.count+1] = rchildren[++i];

        for(; i<m ; i++)
        {    right.childrenq[right.count]=rchildren[i];
            right.key[right.count++]=rkey[i];
        }

        right.children[right.count+1]=rchildren[++i];

        //fin de split

        indexNode<Tk> dummy;
        int next_logical_index = calculate_index(dummy);
        write_index_node_in_pos(file , left , index);
        write_page_node_in_pos(file , right , next_logical_index);

        tuple<int , int , Tk > lrk={index , next_logical_index , new_key};

        return lrk;
    }
    //obtiene el puntero en child para la recursion
    int get_next_index(Tk  keys[m-1], int size, Tk key ,  int children[m] ){
       int pos =   int(upper_bound(keys  , keys + size , key) - &keys[0]) ;
        return children[pos-1];

    }

    int calculate_index(dataPage<Tk> ){ //calculate next logical index or assign you a free space from the free list
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
        int total = int(file.tellg())-metadata_size/data_length;
        return total+1;

    }



    int calculate_index(indexNode<Tk> ){

        fstream file(indexfile , ios::in|ios::out);
        indexNode<Tk> b;

        if(index_fl_top !=-1)
        {
            int ans = data_fl_top;
            b = read_index_node_in_pos(file , index_fl_top);
            index_fl_top = b.next_removed;
            file.seekp(0,ios::beg);
            file.write((char*)(&index_fl_top), metadata_size);
            return ans;
        }

        file.seekg(0 , ios::end);
        int total = int(file.tellg())-metadata_size/index_length;
        return total+1;

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
    int get_total_size(fstream& file ){
        file.seekg(0 , ios::end);
        int total = int(file.tellg()) - metadata_size;
        int object_size = sizeof(T);
        return total/object_size;
    }

    void create_new_files(){

        indexNode<Tk> default_node;
        default_node.is_root=true;
        default_node.is_last = true;

        default_node.showdata();


        dataPage<Tk> default_page_node;

        default_page_node.showdata();
        //Crear datafile si no existiera
        fstream file(datafile , ios::binary|ios::in|ios::out);

        if(!file.is_open())
        {
            file.open(datafile, ios::binary|ios::out);
            int default_free_list_top = -1;
            file.seekp(0, ios::beg);
            file.write((char*)(&default_free_list_top) , metadata_size);
            file.seekp(metadata_size , ios::beg);
            file.write((char*)(&default_node) , index_length); //inicializa la root
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
            file2.seekp(metadata_size , ios::beg);
            file2.write((char*)(&default_page_node) ,data_length);
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


    indexNode<Tk> read_index_node_in_pos(fstream& file, int pos)
    {
        indexNode<Tk> index_node;
        file.seekg(metadata_size+ pos*index_length, ios::beg);
        file.read((char*)(&index_node) , index_length);

        return index_node;
    }
    void write_index_node_in_pos(fstream& file ,const indexNode<Tk> index_node , int pos){

        file.seekp(metadata_size + pos*index_length , ios::beg);
        file.write((char*)(&index_node) , index_length);

    }

    dataPage<Tk> read_page_node_in_pos(fstream &file  , int pos){
        dataPage<Tk> data_node;
        file.seekg(metadata_size+ pos*data_length, ios::beg);
        file.read((char*)(&data_node) , data_length);
        return data_node;

    }
    void write_page_node_in_pos(fstream& file ,dataPage<Tk>& data_node , int pos){
        file.seekg(metadata_size+ pos*data_length, ios::beg);
        file.read((char*)(&data_node) , data_length);

    }




};
// Función que genera y devuelve un vector de records
template<typename Tk>
std::vector<Record<Tk>> generateRecords() {
    // Inicializar generador de números aleatorios
    std::srand(std::time(0));

    // Lista de keys
    Tk keys[] = {27, 16, 24, 10, 37, 15, 25, 38, 32, 8, 31, 2, 45, 6, 36};
    int num_keys = sizeof(keys) / sizeof(keys[0]);

    // Lista de nombres de personajes (Cinderella Girls, Madoka Magica, Bakemonogatari)
    const char* names[] = {
            "Chika Yokoyama",    // Cinderella Girls
            "Mai Fukuyama",      // Cinderella Girls
            "Kozue Yusa",        // Cinderella Girls
            "Koharu Koga",       // Cinderella Girls
            "Momoka Sakurai",    // Cinderella Girls
            "Yukimi Sajo",       // Cinderella Girls
            "Chie Sasaki",       // Cinderella Girls
            "Arisu Tachibana",   // Cinderella Girls
            "Haru Yuuki",        // Cinderella Girls
            "Madoka Kaname",     // Madoka Magica
            "Homura Akemi",      // Madoka Magica
            "Mami Tomoe",        // Madoka Magica
            "Hitagi Senjougahara",// Bakemonogatari
            "Mayoi Hachikuji",   // Bakemonogatari
            "Tsubasa Hanekawa"   // Bakemonogatari
    };

    // Crear vector para almacenar los registros
    std::vector<Record<Tk>> records;

    for (int i = 0; i < num_keys; ++i) {
        Record<Tk> rec;
        rec.key = keys[i];
        rec.code = std::rand() % 9000 + 1000;  // Generar código aleatorio de 4 dígitos
        strncpy(rec.name, names[i], sizeof(rec.name) - 1);  // Asignar nombre correspondiente
        rec.name[sizeof(rec.name) - 1] = '\0';  // Asegurar que la cadena esté terminada en null

        records.push_back(rec);
    }

    return records;  // Devolver el vector de records
}

int main(){

//   vector<Record<int>> r = generateRecords<int>();

   BplusTree<int> test;
//   test.print_all();






}