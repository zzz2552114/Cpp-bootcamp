#include<vector>
#include<memory>
#include<algorithm>

namespace tree5{

    struct Node;

    using uptr = std::unique_ptr<Node>;

    struct Node{
        int val;
        uptr left;
        uptr right;
        Node(int x):val(x),left(nullptr),right(nullptr){}
    };




    class BinaryTree{
    public:

        void Insert(int x){
            // 这是递归写法，用到了辅助函数
            insert(x,root);
        }

        bool Contains(int x) const{
            // 这里展示迭代写法
            // 由于是 const，所以我可以直接裸指针遍历
            const Node* curr = root.get();
            while(curr){
                if(curr->val==x) return 1;
                else{
                    if(curr->val > x) curr = (curr -> left).get();
                    // 注意这里 const Node* 是说不能改变指针指向的地址里的内容
                    // 而不是说不能改变指针的指向
                    else curr = (curr -> right).get();
                }
            }
            return 0;
        }


        int Height() const{
            // 我还是更习惯递归，不用辅助函数的方法就是传默认参数
            // 但是函数默认参数是没有 this 的信息的，挺麻烦
            // 所以还是写辅助函数比较好
            return height(root);
        }

        int Size() const{
            return size(root);
        }

        std::vector<int> InOrder() const{
            // 中序遍历没必要递归，后序再递归吧
            // 数组模拟一下栈
            
            std::vector<int> ans;
            std::vector<const Node*> stack;
            if(root==nullptr) return ans;
            const Node* curr = root.get();
            while(curr || !stack.empty()){
                while(curr){
                    stack.push_back(curr);
                    curr = curr->left.get();
                }
                curr = stack.back();
                stack.pop_back();
                ans.push_back(curr->val);
                curr = curr->right.get();
            }
            return ans;

        }    

        bool Empty() const{
            return root==nullptr;
        }

        uptr root = nullptr;

    private:
        void insert(int x,uptr& nd){
            if (!nd)
                nd = std::make_unique<Node>(x);
            else
            {
                if (x < nd->val)
                    insert(x,nd->left);
                else insert(x,nd->right);
            }
        }

        int height(const uptr& nd) const
        {
            if(!nd) return 0;
            return 1 + std::max(height(nd->left),height(nd->right));
        }

        int size(const uptr &nd) const
        {
            if (!nd)
                return 0;
            return 1 + size(nd->left) + size(nd->right);
        }

    };

}